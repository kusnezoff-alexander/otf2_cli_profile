#pragma once

#include <otf2/OTF2_GeneralDefinitions.h>
#include <boost/container_hash/hash.hpp>
#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <vector>

// forward declaration
namespace definitions {
	struct File;

}

struct IoAccess {
	OTF2_TimeStamp start_time;
	OTF2_TimeStamp end_time;
	uint64_t fpos;
	uint64_t size;
};

using Fpos = uint64_t;
using StartTime = OTF2_TimeStamp;
/** vector of (endTime, (fpos,request_size,startTime)) */
// using IOAccesses = std::vector<std::pair<OTF2_TimeStamp, std::tuple<Fpos,size_t,StartTime>>>;
using IOAccesses = std::vector<IoAccess>;
using TimeInterval = std::pair<OTF2_TimeStamp,OTF2_TimeStamp>;

namespace access_pattern_detection {

/** Nr of accesses from which on we can start speaking of Access Patterns
 * since eg a single access can not showcase an access pattern
 * - WARNING: do NOT make this value <3, some algorithms (eg `detect_local_access_pattern()`) rely on it being >=3
 */
static const short NR_ACCESSES_THRESHOLD = 3;
/** Threshold for which we consdier offsets/sizes to be (almost) equal
 *	eg if 95% of all sizes requested by a location are equal then we consider
 *	the access pattern to be `EQUALLY_SIZED`
 */
static const float ALMOST_EQUAL_THRESHOLD = 0.95;


/**
 * @brief Access Pattern which are supported during detection, determined by requested Offsets&Sizes
 *
 * - by offsets:
 * - by sizes: `EQUALLY_SIZED` vs non-equally sized (by our definition: 95% of all requested sizes are equal to account
 *   for eg remainding bytes on last read/write)
 *
 * TODO: add global access patterns (rewrite: `EQUALLY_SIZED`&`GLOBAL`/`LOCAL` as own bit?
 */
enum class AccessPattern {
	/// if nr of I/O requests is to few to determine an Access Pattern
	/// (threshold is set in @ref NR_ACCESSES_THRESHOLD
	NONE,
	/// No holes in btw accesses (`last_offset + current_size = next_offset`)
	CONTIGUOUS,
	/// Accesses with equal distances btw offsets BUT without reading all bytes btw the two
	/// offsets (which would be a CONTIGUOUS access pattern)
	STRIDED,
	/// None of the above
	RANDOM,
};


#define ENUM_TO_STRING_CASE(e) case e: return #e;
/** @brief Tuns Access Pattern into a string (to eg write it to the json file) */
constexpr const char* access_pattern_to_string(AccessPattern access_pattern)
{
	switch (access_pattern) {
		ENUM_TO_STRING_CASE(AccessPattern::NONE)
		ENUM_TO_STRING_CASE(AccessPattern::CONTIGUOUS)
		ENUM_TO_STRING_CASE(AccessPattern::STRIDED)
		ENUM_TO_STRING_CASE(AccessPattern::RANDOM)
	}
}

struct pair_hash {
    std::size_t operator()(const std::pair<uint64_t,uint64_t>& p) const noexcept {
        std::size_t seed = 0;
        boost::hash_combine(seed, p.first);
        boost::hash_combine(seed, p.second);
        return seed;
    }
};


struct PatternStatistics {
	uint64_t io_size;
	uint64_t ticks_spent;

	PatternStatistics& operator+=(const PatternStatistics& other) {
        io_size += other.io_size;
        ticks_spent += other.ticks_spent;
        return *this;
    }

    PatternStatistics operator+(const PatternStatistics& other) const {
        PatternStatistics tmp = *this;
        tmp += other;
        return tmp;
    }
};

struct AnalysisResult {
	std::unordered_map<TimeInterval, AccessPattern, pair_hash> pattern_per_timeinterval;
	std::unordered_map<AccessPattern, PatternStatistics> stats_per_pattern;
};

/**
 * @brief Returns access pattern based on the sequentially ordered offsets and I/O sizes requested by the single location
 *
 *
 *
 * @param io_accesses 	An I/O access (fpos,size) contains a requested size at a certain file position `fpos` in the file
 * 						NOTE: I/O accesses are expected to be ordered in the same time sequence they have been performed in
 * 						- ! make sure its size is >1
 * @returns
 *		- @ref AccessPattern::NONE if there are less than @ref NR_ACCESSES_THRESHOLD I/O accesses inside io_accesses
 *		- @ref AccessPattern::CONTIGUOUS if the next fpos continues where the previous I/O left off (after accessing `size` bytes from
 *											the previous fpos)
 *		- @ref AccessPattern::STRIDED if the distances between file positions are (almost) equi distant (see @ref ALMOST_EQUAL_THRESHOLD)
 *		- @ref AccessPattern::RANDOM if none of the above conditions are met
 *		- `EQUALLY_SIZED`-variants if @ref ALMOST_EQUAL_THRESHOLD of differences between offsets are equal
 */
AnalysisResult detect_local_access_pattern(IOAccesses& io_accesses);

/**
 * @brief Returns access pattern based on the sequentially ordered offsets and I/O sizes requested by all locations onto a single file
 *
 * @param offsets (Absolute) Offsets in the same ordered they have been requested by the location via e.g. seeking
 * @param size Request I/O sizes in the same ordered they have been requested by the location during read/write
 */
AccessPattern detect_global_access_pattern(definitions::File fh);

} // namespace access_pattern_detection
