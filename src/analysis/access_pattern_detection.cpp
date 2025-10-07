//! Utilities for computing Access-Patterns (mostly using offsets & sizes at which locations have accessed the file)
//! - *local access pattern* = on a single file, per location
//! - *global access pattern* = on a single file, viewed across all accessing locations
#include "access_pattern_detection.h"

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <ranges>
#include <unordered_map>
#include <utility>

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

AccessPattern detect_local_access_pattern(std::vector<std::pair<uint64_t,size_t>>& io_accesses)
{
	std::vector<uint64_t> file_positions;
	std::ranges::transform(io_accesses, std::back_inserter(file_positions),
						   [](auto const& p) { return p.first; });
	std::vector<uint64_t> sizes;
	std::ranges::transform(io_accesses, std::back_inserter(sizes),
						   [](auto const& p) { return p.second; });

	bool is_equally_sized, is_equi_distant_access;
	uint64_t size, fpos_diff; // most frequent size & difference inbetween file positions

	// NOTE: for less then `NR_ACCESSES_THRESHOLD` requests we can't really speak of an access pattern
	if (io_accesses.size() < NR_ACCESSES_THRESHOLD)
	 	return AccessPattern::NONE;
	else
		std::tie(is_equally_sized, size) = is_equally_sized_access(sizes);

	// Check if strided (=equal distances btw fposs)
	std::tie(is_equi_distant_access, fpos_diff) = has_equi_distant_fpos(file_positions);

	if (is_equi_distant_access)
		if (is_equally_sized) {
			// if the sizes exactly fill the whole space btw two fposs we still consider the access to be contiguous
			// (=sizes are the same as the distance btw two consecutive fposs)
			return size == fpos_diff ? AccessPattern::CONTIGUOUS_EQUALLY_SIZED : AccessPattern::STRIDED_EQUALLY_SIZED;
		}
		else
			return AccessPattern::STRIDED;
	else {
		if (is_equally_sized)
			// the sizes could still be (almost) equal, but at random fposs
			return AccessPattern::RANDOM_EQUALLY_SIZED;
		else {
			// sizes are not (almost) equal AND fpos are not (almost) equidistant
			// -> either `CONTIGUOUS` (next fpos lies directly behind previous requested size) or `RANDOM`
			auto next_fpos_if_contiguous = io_accesses[0].first; // stores next fpos it the access was contiguous
			for (auto& [fpos, size]: io_accesses) {
				if (fpos != next_fpos_if_contiguous)
					// this access was not contiguous
					return AccessPattern::RANDOM;

				// if fpos has been changed update it here
				next_fpos_if_contiguous = fpos + size;
			}

			// all fposs&sizes seem to have been consistent with a contiguous access
			return AccessPattern::CONTIGUOUS;
		}
	}
}

AccessPattern detect_global_access_pattern(definitions::File& file)
{
	// analyze all IoHandles that were used to perform I/O on `file`
}

} // namespace access_pattern_detection
