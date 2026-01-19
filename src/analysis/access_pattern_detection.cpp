//! Utilities for computing Access-Patterns (mostly using offsets & sizes at which locations have accessed the file)
//! - *local access pattern* = on a single file, per location
//! - *global access pattern* = on a single file, viewed across all accessing locations
#include "access_pattern_detection.h"

#include <algorithm>
#include <boost/container_hash/hash.hpp>
#include <cassert>
#include <cstdint>
#include <numeric>
#include <unordered_map>
#include <utility>
#include <boost/functional/hash.hpp>

#include "all_data.h"
#include "definitions.h"
#include "otf2/OTF2_GeneralDefinitions.h"

using namespace std;

namespace access_pattern_detection {

/** Helper function that takes in the last `NR_ACCESSES_THRESHOLD` I/O Accesses and determines their
 * AccessPattern
 */
AccessPattern detect_access_pattern_from_3_acccesses(IOAccesses& io_accesses_ringbuffer, short ringbuffer_start) {
	assert(io_accesses_ringbuffer.size()==3);
	IOAccesses io_accesses(3);
	for(short i=0; i<3; ++i){
		io_accesses[i] = io_accesses_ringbuffer[(ringbuffer_start+i)%3];
	}
	bool is_contiguous = (io_accesses[0].fpos+io_accesses[0].size == io_accesses[1].fpos)
		&& (io_accesses[1].fpos+io_accesses[1].size == io_accesses[2].fpos);
	if (is_contiguous) {
		return AccessPattern::CONTIGUOUS;
	}

	bool is_strided =  (io_accesses[1].fpos - io_accesses[0].fpos) == (io_accesses[2].fpos - io_accesses[1].fpos);
	if (is_strided) {
		return AccessPattern::STRIDED;
	}

	return AccessPattern::RANDOM;
}

int mod(int a, int b) {
    return (a % b + b) % b;
}

void init_stats(PatternStatistics& stats, IOAccesses& accesses)
{
	for(auto& io: accesses) {
		stats += PatternStatistics(io.size, io.duration); // added to result when `io_accesses` are checked in
	}
}

AnalysisResult detect_local_access_pattern(IOAccesses& io_accesses)
{
	if (io_accesses.empty())
		return AnalysisResult({}, {});

	std::unordered_map<TimeInterval, AccessPattern, pair_hash> pattern_per_timeinterval;
	// NOTE: for less then `NR_ACCESSES_THRESHOLD` requests we can't really speak of an access pattern
	if (io_accesses.size() < NR_ACCESSES_THRESHOLD) {
		pattern_per_timeinterval[std::pair(io_accesses[0].start_time_ns, io_accesses.back().end_time_ns)] = AccessPattern::NONE;
		uint64_t io_size = std::accumulate(io_accesses.begin(), io_accesses.end(), 0,
				[](uint64_t acc, auto& io) {
					return acc + io.size;
				});;
		uint64_t ticks_spent = std::accumulate(io_accesses.begin(), io_accesses.end(), 0,
				[](uint64_t acc, auto& io) {
					return acc + io.duration;
				});;
		PatternStatistics stats (io_size, ticks_spent);
		std::unordered_map<AccessPattern, PatternStatistics> stats_per_pattern = {
			{AccessPattern::NONE, stats}
		};
		AnalysisResult result (pattern_per_timeinterval, stats_per_pattern);
		return result;
	}

	std::unordered_map<AccessPattern, PatternStatistics> stats_per_pattern = {
		{AccessPattern::NONE, PatternStatistics(0,0)},
		{AccessPattern::CONTIGUOUS, PatternStatistics(0,0)},
		{AccessPattern::STRIDED, PatternStatistics(0,0)},
		{AccessPattern::RANDOM, PatternStatistics(0,0)},
	};


	// these vars will be needed
	OTF2_TimeStamp interval_start = io_accesses.front().start_time_ns;
	OTF2_TimeStamp last_timestamp = io_accesses.back().end_time_ns;
	bool is_equi_distant;								// used to determine whether access pattern is strided
	OTF2_TimeStamp last_x_accesses_prev_interval_end;
	IOAccesses last_x_accesses {
		// insert first three io-accesses
		io_accesses[0], io_accesses[1], io_accesses[2]
	};													// tracks the access pattern of the last 3 I/O-accesses (eg to check whether diff btw fpos / size / ...
														// of the current IoAccess changed (compared to previous I/O accesses)
	short id_into_last_x_accesses = 2;					// pretend `last_x_accesses` is a ringbuffer (with 2 elements already inserted) ->so first elem is inserted at [0]

	uint64_t next_fpos_if_contiguous = last_x_accesses.back().fpos + last_x_accesses.back().size; 	// used to determine if access pattern is still contiguous (by comparing to next fpos)
	uint64_t nr_io_access_in_current_access_pattern = last_x_accesses.size();
	uint64_t prev_fpos = last_x_accesses.back().fpos;
	uint64_t last_fpos_distance = last_x_accesses.back().fpos - last_x_accesses[1].fpos;			// used to determine whether access pattern is (still) equidistant (for STRIDED) access

	PatternStatistics curr_stats{}; // keeps track of IO_Size & Ticks_spent until actual pattern is clear
	init_stats(curr_stats, last_x_accesses);
	AccessPattern curr_pattern = detect_access_pattern_from_3_acccesses(last_x_accesses, 0);
	bool do_start_new_interval = false;					// interval with different accesss pattern -> track separately

	// ! GOAL: single pass over all accesses (which can be a lot)
	// TODO: output AccessPattern for exactly 3 IoAccesses
	for(int i=3; i<io_accesses.size(); ++i) {
		if (io_accesses[i].is_meta) // meta operations don't contribute to file access pattern
			continue;

		auto io = io_accesses[i];

		id_into_last_x_accesses = (id_into_last_x_accesses+1) % NR_ACCESSES_THRESHOLD;
		last_x_accesses_prev_interval_end = last_x_accesses[id_into_last_x_accesses].end_time_ns;
		last_x_accesses[id_into_last_x_accesses] = io;

		last_fpos_distance = last_x_accesses[mod(id_into_last_x_accesses-1,NR_ACCESSES_THRESHOLD)].fpos
								- last_x_accesses[mod(id_into_last_x_accesses-2, NR_ACCESSES_THRESHOLD)].fpos;

		// init things for new interval (with potentially new access pattern)
		if (do_start_new_interval) {
			curr_pattern = AccessPattern::CONTIGUOUS;			// start with CONTIGUOUS as default bc it is the most strict one
			next_fpos_if_contiguous = io_accesses[i].fpos;
			curr_stats = PatternStatistics(0, 0);
			nr_io_access_in_current_access_pattern = 0; // counts how many I/O accesses were assigned to the current access pattern

			interval_start = io.start_time_ns;

			is_equi_distant = true; // no access yet
			do_start_new_interval = false;
		}
		curr_stats += PatternStatistics(io.size, io.duration); 	// added to result when `io_accesses` are checked in
																				// TODO: for time take actual time spent in io-access !

		// implement Transition btw AccessPattern states
		switch (curr_pattern) {
			case AccessPattern::CONTIGUOUS:
				// `CONTIGUOUS->STRIDED` possible if all accesses were equidistant so far, else `CONTIGUOUS->RANDOM`
				// (`do_start_new_interval==true` means this is the only access analyzed so far, so let's be optimistic and see if coming
				// I/O accesses will show that the access pattern is indeed contiguous
				if (!do_start_new_interval && io.fpos!=next_fpos_if_contiguous) {
					// if we have already enough access in this pattern let's save it as being CONTIGUOUS..
					if (nr_io_access_in_current_access_pattern > NR_ACCESSES_THRESHOLD) {
						// END --- check in this contiguous interval
						pattern_per_timeinterval[std::pair(interval_start, io.end_time_ns)] = AccessPattern::CONTIGUOUS;
						stats_per_pattern[AccessPattern::CONTIGUOUS] += curr_stats;
						do_start_new_interval = true;
						break;
					} else if (is_equi_distant) {
						// CONTIGUOUS -> STRIDED
						// could still be strided
						curr_pattern = AccessPattern::STRIDED;
					} else {
						// CONTIGUOUS -> RANDOM
						curr_pattern = AccessPattern::RANDOM;
					}
				} else {
					// CONTIGUOUS -> CONTIGUOUS
					// we are still in CONTIGUOUS access pattern
					next_fpos_if_contiguous = io.fpos + io.size; 	// next fpos is determined by currently read/written size
															// (if AccessPattern is contiguous)

				}
				nr_io_access_in_current_access_pattern++;
				break;
			case AccessPattern::STRIDED:
			{
				// `STRIDED->CONTIGUOUS` not possible (only vice versa), `STRIDED->RANDOM`: if not equidistant anymore
				if (io.end_time_ns == last_timestamp) {
					// END --- check in all io_accesses
					// if only the last access is not equidistant the whole interval still counts as being accessed via STRIDED pattern
					pattern_per_timeinterval[std::pair(interval_start,io.end_time_ns)] = AccessPattern::STRIDED;
					stats_per_pattern[AccessPattern::STRIDED] += curr_stats;
					nr_io_access_in_current_access_pattern = 0;
					break;
				}

				// check if `STRIDED -> CONTIGUOUS` is possible
				AccessPattern live_pattern = detect_access_pattern_from_3_acccesses(last_x_accesses, id_into_last_x_accesses);
				if (live_pattern == AccessPattern::CONTIGUOUS) {
					curr_pattern = AccessPattern::CONTIGUOUS;
					nr_io_access_in_current_access_pattern = NR_ACCESSES_THRESHOLD;

					interval_start = last_x_accesses[mod(id_into_last_x_accesses-1,NR_ACCESSES_THRESHOLD)].start_time_ns; // update when the interval of those last `x` accesses started
																											   // (=first of those recorded x events)
					// leave `is_equi_distant==true` (STRIDED implies equi-distant)
					assert(is_equi_distant);

					if (nr_io_access_in_current_access_pattern > NR_ACCESSES_THRESHOLD*2) {
						// there were still some strided accesses before the contiguous ones -> check them in !
						pattern_per_timeinterval[std::pair(interval_start, last_x_accesses_prev_interval_end)] = AccessPattern::STRIDED;
						stats_per_pattern[AccessPattern::STRIDED] += curr_stats;
					}
					break;
				}

				if (io.fpos - last_x_accesses[mod(id_into_last_x_accesses-1,NR_ACCESSES_THRESHOLD)].fpos == last_fpos_distance) {
					// STRIDED -> STRIDED
					// we are still equidistant (INVARIANT: `is_equi_distant==true` only if `curr_pattern == AccessPattern::STRIDED`)
					is_equi_distant = true;
					nr_io_access_in_current_access_pattern +=1;
				} else {
					// next access is not STRIDED anymore -> start new interval (or if <NR_ACCESSES_THRESHOLD count as RANDOM pattern)
					if (nr_io_access_in_current_access_pattern < NR_ACCESSES_THRESHOLD) {
						// STRIDED -> RANDOM
						is_equi_distant = false;
						curr_pattern = AccessPattern::RANDOM;
					} else {
						// INTERVAL FINISHED: check in
						bool is_last_strided = i==io_accesses.size()-1; // if this is last io: also belongs to strided i guess (last acc might be limited by file size)
						auto end_time = is_last_strided ? io.end_time_ns : io_accesses[i-1].end_time_ns;
						pattern_per_timeinterval[std::pair(interval_start,end_time)] = AccessPattern::STRIDED;
						stats_per_pattern[AccessPattern::STRIDED] += curr_stats;
						do_start_new_interval = true;

						if (!is_last_strided)
							i--; // make this last io be part of next pattern
						continue;
					}
				}
				break;
			}
				case AccessPattern::RANDOM:
			{
				AccessPattern live_pattern = detect_access_pattern_from_3_acccesses(last_x_accesses, id_into_last_x_accesses);

				if (live_pattern!=AccessPattern::RANDOM) {
					// RANDOM -> CONTIGUOUS | STRIDED
					curr_pattern = live_pattern;
					nr_io_access_in_current_access_pattern = NR_ACCESSES_THRESHOLD; // all elements in `last_x_accesses` indicate `live_pattern`

					// setup vars for this new access pattern
					interval_start = last_x_accesses[mod(id_into_last_x_accesses-1,NR_ACCESSES_THRESHOLD)].start_time_ns; // update when the interval of those last `x` accesses started
																											   // (=first of those recorded x events)
					is_equi_distant = curr_pattern==AccessPattern::STRIDED || (
							// if first two accesses were equidistant all of them must have been
							(last_x_accesses[mod(id_into_last_x_accesses-1,NR_ACCESSES_THRESHOLD)].size - last_x_accesses[mod(id_into_last_x_accesses-2,NR_ACCESSES_THRESHOLD)].size) // distance btw 1st&2nd access
						==	(last_x_accesses[mod(id_into_last_x_accesses-2,NR_ACCESSES_THRESHOLD)].size - last_x_accesses[mod(id_into_last_x_accesses-3,NR_ACCESSES_THRESHOLD)].size) // distance btw 2nd&3rd access
					);

					if (nr_io_access_in_current_access_pattern > NR_ACCESSES_THRESHOLD*2) {
						// there were still some random accesses before -> check them in !
						pattern_per_timeinterval[make_pair(interval_start, last_x_accesses_prev_interval_end)] = AccessPattern::RANDOM;
						stats_per_pattern[AccessPattern::RANDOM] += curr_stats;
					}
				} else {
					// RANDOM -> RANDOM
					nr_io_access_in_current_access_pattern += 1;
				}
				break;
			}
			case AccessPattern::NONE:
				cerr << "AccessPattern::NONE should have been filtered out previously\n";
				abort();  // Immediately terminates the program
				break;
		}
	}

	if (nr_io_access_in_current_access_pattern>0) {
		// check in remaining accesses
		auto last_io = last_x_accesses[mod(id_into_last_x_accesses,NR_ACCESSES_THRESHOLD)];
		pattern_per_timeinterval[make_pair(interval_start,last_io.end_time_ns)] = curr_pattern;
		stats_per_pattern[curr_pattern] += curr_stats;
	}

	return AnalysisResult(pattern_per_timeinterval, stats_per_pattern);
}

AccessPattern detect_global_access_pattern(AllData& alldata, definitions::File* file)
{
	// analyze all IoHandles that were used to perform I/O on `file`

	// sequentialize accesses from all I/O handles sorted by timestamp and then proceed with *local access pattern detection*
	for (auto& handle: file->io_handles){
		auto* ioh = alldata.definitions.iohandles.get(handle);
		std::cout << ioh->io_accesses.size() << std::endl;
	}
	throw std::logic_error("TODO: not implemented");
}

} // namespace access_pattern_detection
