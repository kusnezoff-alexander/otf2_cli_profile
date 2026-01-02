#include <gtest/gtest.h>
#include <otf2/OTF2_GeneralDefinitions.h>
#include <utility>
#include "access_pattern_detection.h"

TEST(AccessPattern, Simple) {
    EXPECT_EQ(1+1, 2);

	// std::vector<std::pair<OTF2_TimeStamp, std::tuple<Fpos,size_t,StartTime>>>
	OTF2_TimeStamp contiguous_start = 0, contiguous_end = 3;
	IOAccesses contiguous_only {
		std::pair(contiguous_end, std::tuple(0, 2, contiguous_start)),
		std::pair(5, std::tuple(2, 2, 10)), // 5-10 ticks: fpos=2, req-size=2
		std::pair(15, std::tuple(2, 2, 20)), // 5-10 ticks: fpos=2, req-size=2
	};
	auto result = access_pattern_detection::detect_local_access_pattern(contiguous_only);

	std::unordered_map<TimeInterval, access_pattern_detection::AccessPattern, access_pattern_detection::pair_hash> CONTIGUOUS_SHOULD = { {std::pair(contiguous_start, contiguous_end), access_pattern_detection::AccessPattern::CONTIGUOUS} };
	EXPECT_EQ(result.pattern_per_timeinterval, CONTIGUOUS_SHOULD);
}

