//! Utilities for computing Access-Patterns (mostly using offsets & sizes at which locations have accessed the file)
//! - *local access pattern* = on a single file, per location
//! - *global access pattern* = on a single file, viewed across all accessing locations
#include "access_pattern_detection.h"

#include <algorithm>
#include <array>
#include <boost/container_hash/hash.hpp>
#include <cstdint>
#include <limits>
#include <numeric>
#include <ranges>
#include <unordered_map>
#include <utility>
#include <boost/functional/hash.hpp>

#include "all_data.h"
#include "definitions.h"
#include "otf2/OTF2_GeneralDefinitions.h"


namespace access_pattern_detection {

/** @brief Check whether a vector of values meets our definition of "almost equal"
 * - see @ref ALMOST_EQUAL_THRESHOLD
 *
 *   @param v non-empty vector
 *   @returns (is_almost_equal,most_frequent_value)
 */
std::pair<bool,uint64_t> is_almost_equal(std::vector<uint64_t>& v)
{
	assert(!v.empty());
    std::unordered_map<int, size_t> freq;

    for (auto x : v) ++freq[x];
    auto [value, max_count] = *std::max_element(
        freq.begin(), freq.end(),
        [](auto& a, auto& b){ return a.second < b.second; }
    );
    bool is_almost_equal = max_count >= v.size() * ALMOST_EQUAL_THRESHOLD;
	return std::make_pair(is_almost_equal, value);
}

/** @brief Checks whether requested fpos have (almost) equal distance inbtw (by our definition: 95% of accesses are equal)
 *
 * @ref AccessPattern
 * @returns (true,<most_frequent_value>) if we consider the requested fpos to have (almost) equal distances
 */
inline std::pair<bool,uint64_t> has_equi_distant_fpos(std::vector<uint64_t>& fpos)
{
	std::vector<uint64_t> fpos_diffs(fpos.size());
	std::adjacent_difference(fpos.begin(), fpos.end(), fpos_diffs.begin());
	fpos_diffs.erase(fpos_diffs.begin()); // first elem is copied from original vec when using `adjacent_difference()`
	return is_almost_equal(fpos_diffs);
}

/** @brief Checks whether requested sizes are (almost) equal (by our definition: 95% of accesses are equal)
 *
 * @ref AccessPattern
 * @returns (true,<most_frequent_value>) if we consider the requested sizes to be (almost) equal
 */
inline std::pair<bool,uint64_t> is_equally_sized_access(std::vector<uint64_t>& sizes)
{
	return is_almost_equal(sizes);
}

/** Helper function that takes in the last `NR_ACCESSES_THRESHOLD` I/O Accesses and determines their
 * AccessPattern
 */
AccessPattern detect_access_pattern_from_x_acccesses(IOAccesses& io_accesses_ringbuffer, short ringbuffer_start) {
	IOAccesses io_accesses;
	for(short i=0; i<NR_ACCESSES_THRESHOLD; ++i){
		io_accesses[i] = io_accesses_ringbuffer[(ringbuffer_start+i)%NR_ACCESSES_THRESHOLD];
	}

	bool is_contiguous = std::adjacent_find(io_accesses.begin(), io_accesses.end(),
			[](auto io1, auto io2){ return std::get<1>(io1.second) + std::get<2>(io1.second) == std::get<1>(io2.second) ;}) > io_accesses.end();
	if (is_contiguous) {
		return AccessPattern::CONTIGUOUS;
	}

	uint64_t distance = std::get<1>(io_accesses[0].second) - std::get<1>(io_accesses[1].second);
	bool is_strided= std::adjacent_find(io_accesses.begin(), io_accesses.end(),
			[&distance](auto io1, auto io2){ return (std::get<1>(io2.second) - std::get<1>(io1.second)) != distance;}) > io_accesses.end();
	if (is_strided) {
		return AccessPattern::STRIDED;
	}

	return AccessPattern::RANDOM;
}

int mod(int a, int b) {
    return (a % b + b) % b;
}

AnalysisResult detect_local_access_pattern(IOAccesses& io_accesses)
{

	std::unordered_map<TimeInterval, AccessPattern, pair_hash> pattern_per_timeinterval;
	// NOTE: for less then `NR_ACCESSES_THRESHOLD` requests we can't really speak of an access pattern
	if (io_accesses.size() < NR_ACCESSES_THRESHOLD) {
		pattern_per_timeinterval[std::pair(io_accesses[0].first, io_accesses.back().first)] = AccessPattern::NONE;
		uint64_t io_size = std::accumulate(io_accesses.begin(), io_accesses.end(), 0,
				[](uint64_t acc, auto& io) {
					return acc + std::get<2>(io.second);
				});;
		uint64_t ticks_spent = io_accesses.back().first - io_accesses[0].first;
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

	AccessPattern curr_pattern = AccessPattern::CONTIGUOUS;
	PatternStatistics curr_stats; // keeps track of IO_Size & Ticks_spent until actual pattern is clear

	// these vars will be needed
	uint64_t next_fpos_if_contiguous; 					 // used to determine if access pattern is still contiguous (by comparing to next fpos)
	uint64_t nr_io_access_in_current_access_pattern;
	OTF2_TimeStamp interval_start;
	OTF2_TimeStamp last_timestamp;
	uint64_t prev_fpos, last_fpos_distance;				// used to determine whether access pattern is (still) equidistant (for STRIDED) access
	bool is_equi_distant;								// used to determine whether access pattern is strided
	OTF2_TimeStamp last_x_accesses_prev_interval_end;
	IOAccesses last_x_accesses {
		// insert some values so `last_fpos_distance` actually makes sense
		std::pair(0, std::tuple(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max())),
		std::pair(0, std::tuple(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max())),
		std::pair(0, std::tuple(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max())),  // this is only here for `last_x_accesses_prev_interval_end`
	};													// tracks the access pattern of the last `NR_ACCESSES_THRESHOLD` I/O-accesses
														// (eg for detecting when we are again in a STRIDED access after having had some random access inbtw)
	short id_into_last_x_accesses = 2;					// pretend `last_x_accesses` is a ringbuffer (with 2 elements already inserted

	bool do_start_new_interval = true;

	// ! GOAL: single pass over all accesses (which can be a lot)
	for(auto& io_access: io_accesses) {
		auto& [end_time, inner] = io_access;
		auto& [fpos, size, start_time] = inner;
		curr_stats += PatternStatistics(size, end_time-start_time); // added to result when `io_accesses` are checked in

		id_into_last_x_accesses = (id_into_last_x_accesses+1) % NR_ACCESSES_THRESHOLD;
		last_x_accesses_prev_interval_end = last_x_accesses[id_into_last_x_accesses].first;
		last_x_accesses[id_into_last_x_accesses] = io_access;

		last_fpos_distance = std::get<1>(last_x_accesses[mod(id_into_last_x_accesses-1,NR_ACCESSES_THRESHOLD)].second)
								- std::get<1>(last_x_accesses[mod(id_into_last_x_accesses-2, NR_ACCESSES_THRESHOLD)].second);

		// init things for new interval (with potentially new access pattern)
		if (do_start_new_interval) {
			curr_pattern = AccessPattern::CONTIGUOUS;			// start with CONTIGUOUS as default bc it is the most strict one
			curr_stats = PatternStatistics(0, 0);
			nr_io_access_in_current_access_pattern = 0; // counts how many I/O accesses were assigned to the current access pattern

			interval_start = end_time;

			is_equi_distant = true; // no access yet
		}

		// implement Transition btw AccessPattern states
		switch (curr_pattern) {
			case AccessPattern::CONTIGUOUS:
				// `CONTIGUOUS->STRIDED` not possible since if all accesses were equidistant so far, else `CONTIGUOUS->RANDOM`
				// (`do_start_new_interval==true` means this is the only access analyzed so far, so let's be optimistic and see if coming
				// I/O accesses will show that the access pattern is indeed contiguous
				if (!do_start_new_interval && fpos!=next_fpos_if_contiguous) {
					// if we have already enough access in this pattern let's save it as being CONTIGUOUS..
					if (nr_io_access_in_current_access_pattern > NR_ACCESSES_THRESHOLD) {
						// END --- check in this contiguous interval
						pattern_per_timeinterval[std::pair(interval_start, end_time)] = AccessPattern::CONTIGUOUS;
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
					next_fpos_if_contiguous = fpos + size; 	// next fpos is determined by currently read/written size
															// (if AccessPattern is contiguous)

				}
				nr_io_access_in_current_access_pattern++;
				break;
			case AccessPattern::STRIDED:
			{
				// `STRIDED->CONTIGUOUS` not possible (only vice versa), `STRIDED->RANDOM`: if not equidistant anymore
				if (end_time == last_timestamp) {
					// END --- check in all io_accesses
					// if only the last access is not equidistant the whole interval still counts as being accessed via STRIDED pattern
					pattern_per_timeinterval[std::pair(interval_start,end_time)] = AccessPattern::STRIDED;
					stats_per_pattern[AccessPattern::STRIDED] += curr_stats;
					break;
				}

				// check if `STRIDED -> CONTIGUOUS` is possible
				AccessPattern live_pattern = detect_access_pattern_from_x_acccesses(io_accesses, id_into_last_x_accesses);
				if (live_pattern == AccessPattern::CONTIGUOUS) {
					curr_pattern = AccessPattern::CONTIGUOUS;
					nr_io_access_in_current_access_pattern = NR_ACCESSES_THRESHOLD;

					interval_start = last_x_accesses[mod(id_into_last_x_accesses-1,NR_ACCESSES_THRESHOLD)].first; // update when the interval of those last `x` accesses started
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

				if (fpos - std::get<1>(last_x_accesses[mod(id_into_last_x_accesses-1,NR_ACCESSES_THRESHOLD)].second)
						== last_fpos_distance) {
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
						pattern_per_timeinterval[std::pair(interval_start,end_time)] = AccessPattern::STRIDED;
						stats_per_pattern[AccessPattern::STRIDED] += curr_stats;
						do_start_new_interval = true;
						continue;
					}
				}
				break;
			}
				case AccessPattern::RANDOM:
			{
				AccessPattern live_pattern = detect_access_pattern_from_x_acccesses(last_x_accesses, id_into_last_x_accesses);

				if (live_pattern!=AccessPattern::RANDOM) {
					// RANDOM -> CONTIGUOUS | STRIDED
					curr_pattern = live_pattern;
					nr_io_access_in_current_access_pattern = NR_ACCESSES_THRESHOLD; // all elements in `last_x_accesses` indicate `live_pattern`

					// setup vars for this new access pattern
					interval_start = last_x_accesses[mod(id_into_last_x_accesses-1,NR_ACCESSES_THRESHOLD)].first; // update when the interval of those last `x` accesses started
																											   // (=first of those recorded x events)
					is_equi_distant = curr_pattern==AccessPattern::STRIDED || (
							// if first two accesses were equidistant all of them must have been
							(std::get<1>(last_x_accesses[mod(id_into_last_x_accesses-1,NR_ACCESSES_THRESHOLD)].second) - std::get<1>(last_x_accesses[mod(id_into_last_x_accesses-2,NR_ACCESSES_THRESHOLD)].second)) // distance btw 1st&2nd access
						==	(std::get<1>(last_x_accesses[mod(id_into_last_x_accesses-2,NR_ACCESSES_THRESHOLD)].second) - std::get<1>(last_x_accesses[mod(id_into_last_x_accesses-3,NR_ACCESSES_THRESHOLD)].second)) // distance btw 2nd&3rd access
					);

					if (nr_io_access_in_current_access_pattern > NR_ACCESSES_THRESHOLD*2) {
						// there were still some random accesses before -> check them in !
						pattern_per_timeinterval[std::pair(interval_start, last_x_accesses_prev_interval_end)] = AccessPattern::RANDOM;
						stats_per_pattern[AccessPattern::RANDOM] += curr_stats;
					}
				} else {
					// RANDOM -> RANDOM
					nr_io_access_in_current_access_pattern += 1;
				}
				break;
			}
			case AccessPattern::NONE:
				std::cerr << "AccessPattern::NONE should have been filtered out previously\n";
				std::abort();  // Immediately terminates the program
				break;
		}
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
}

} // namespace access_pattern_detection
