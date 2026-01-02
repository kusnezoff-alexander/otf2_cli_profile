#include <gtest/gtest.h>
#include <otf2/OTF2_GeneralDefinitions.h>
#include <utility>
#include "access_pattern_detection.h"

TEST(AccessPattern, Simple) {
    EXPECT_EQ(1+1, 2);

	// std::vector<std::pair<OTF2_TimeStamp, std::tuple<Fpos,size_t,StartTime>>>
	OTF2_TimeStamp contiguous_start = 0, contiguous_end = 3;
	IOAccesses contiguous_only {
		IoAccess {0, 3, 0, 5},
		IoAccess {8, 30, 5, 1},
		IoAccess {31, 33, 6, 67},
		IoAccess {100, 130, 73, 5},
		IoAccess {131, 132, 78, 10},
		IoAccess {132, 135, 88, 5},
	};
	auto result = access_pattern_detection::detect_local_access_pattern(contiguous_only);

	std::unordered_map<TimeInterval, access_pattern_detection::AccessPattern, access_pattern_detection::pair_hash> CONTIGUOUS_SHOULD = { {std::pair(0, 135), access_pattern_detection::AccessPattern::CONTIGUOUS} };
	EXPECT_EQ(result.pattern_per_timeinterval, CONTIGUOUS_SHOULD);
}

