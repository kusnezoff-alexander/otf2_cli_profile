#pragma once

#include <otf2/OTF2_GeneralDefinitions.h>
#include <cassert>
#include <cstdint>
#include <vector>

// forward declaration
namespace definitions {
	struct File;

}
using Fpos = uint64_t;
using IOAccesses = std::vector<std::pair<OTF2_TimeStamp, std::pair<Fpos,size_t>>>;

namespace access_pattern_detection {

/** Nr of accesses from which on we can start speaking of Access Patterns
 * since eg a single access can not showcase an access pattern
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
	/// `CONTIGUOUS` and all sizes are equally sized
	/// @note This means that the offsets were equi-distant (like in the STRIDED case)
	CONTIGUOUS_EQUALLY_SIZED,
	/// Accesses with equal distances btw offsets BUT without reading all bytes btw the two
	/// offsets (which would be a CONTIGUOUS access pattern)
	STRIDED,
	/// Strided (=equal distances btw offsets) AND with equally sized requests
	STRIDED_EQUALLY_SIZED,
	/// Offsets are distributed randomly, but all requested sizes are equal
	RANDOM_EQUALLY_SIZED,
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
		ENUM_TO_STRING_CASE(AccessPattern::CONTIGUOUS_EQUALLY_SIZED)
		ENUM_TO_STRING_CASE(AccessPattern::STRIDED)
		ENUM_TO_STRING_CASE(AccessPattern::STRIDED_EQUALLY_SIZED)
		ENUM_TO_STRING_CASE(AccessPattern::RANDOM_EQUALLY_SIZED)
		ENUM_TO_STRING_CASE(AccessPattern::RANDOM)
	}
}

/**
 * @brief Returns access pattern based on the sequentially ordered offsets and I/O sizes requested by the single location
 *
 *
 *
 * @param io_accesses 	An I/O access (fpos,size) contains a requested size at a certain file position `fpos` in the file
 * 						NOTE: I/O accesses are expected to be ordered in the same time sequence they have been performed in
 * @returns
 *		- @ref AccessPattern::NONE if there are less than @ref NR_ACCESSES_THRESHOLD I/O accesses inside io_accesses
 *		- @ref AccessPattern::CONTIGUOUS if the next fpos continues where the previous I/O left off (after accessing `size` bytes from
 *											the previous fpos)
 *		- @ref AccessPattern::STRIDED if the distances between file positions are (almost) equi distant (see @ref ALMOST_EQUAL_THRESHOLD)
 *		- @ref AccessPattern::RANDOM if none of the above conditions are met
 *		- `EQUALLY_SIZED`-variants if @ref ALMOST_EQUAL_THRESHOLD of differences between offsets are equal
 */
AccessPattern detect_local_access_pattern(IOAccesses& io_accesses);


/**
 * @brief Returns access pattern based on the sequentially ordered offsets and I/O sizes requested by all locations onto a single file
 *
 * @param offsets (Absolute) Offsets in the same ordered they have been requested by the location via e.g. seeking
 * @param size Request I/O sizes in the same ordered they have been requested by the location during read/write
 */
AccessPattern detect_global_access_pattern(definitions::File fh);

} // namespace access_pattern_detection
