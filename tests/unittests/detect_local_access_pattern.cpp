#include <gtest/gtest.h>
#include <otf2/OTF2_GeneralDefinitions.h>
#include <utility>
#include "access_pattern_detection.h"

using namespace access_pattern_detection;

TEST(AccessPattern, Simple) {

	IOAccesses contiguous_only {
		IoAccess {0, 3, 0, 5, 3, false},
		IoAccess {8, 30, 5, 1, 7, false},
		IoAccess {31, 33, 6, 67, 3, false},
		IoAccess {100, 130, 73, 5, 14, false},
		IoAccess {131, 132, 78, 10, 27, false},
		IoAccess {132, 135, 88, 5, 33, false},
	};
	auto result = access_pattern_detection::detect_local_access_pattern(contiguous_only);
	std::unordered_map<TimeInterval, access_pattern_detection::AccessPattern, access_pattern_detection::pair_hash> CONTIGUOUS_SHOULD_PATTERN = { {std::pair(0, 135), access_pattern_detection::AccessPattern::CONTIGUOUS} };
	std::unordered_map<AccessPattern, PatternStatistics> CONTIGUOUS_SHOULD_STATS = { {AccessPattern::NONE, PatternStatistics{0, 0}}, {AccessPattern::CONTIGUOUS, PatternStatistics{93, 87}},
		{AccessPattern::STRIDED, PatternStatistics{0, 0}}, {AccessPattern::RANDOM, PatternStatistics{0, 0}} };
	EXPECT_EQ(result.pattern_per_timeinterval, CONTIGUOUS_SHOULD_PATTERN);
	EXPECT_EQ(result.stats_per_pattern, CONTIGUOUS_SHOULD_STATS);

	IOAccesses strided_only {
		IoAccess {0, 3, 1000, 5, 3, false},
		IoAccess {8, 30, 2000, 1, 7, false},
		IoAccess {31, 33, 3000, 67, 3, false},
		IoAccess {100, 130, 4000, 5, 14, false},
		IoAccess {131, 132, 5000, 10, 27, false},
		IoAccess {132, 135, 6000, 5, 33, false},
	};
	result = access_pattern_detection::detect_local_access_pattern(strided_only);
	std::unordered_map<TimeInterval, access_pattern_detection::AccessPattern, access_pattern_detection::pair_hash> STRIDED_SHOULD_PATTERN = { {std::pair(0, 135), access_pattern_detection::AccessPattern::STRIDED} };
	std::unordered_map<AccessPattern, PatternStatistics> STRIDED_SHOULD_STATS = { {AccessPattern::NONE, PatternStatistics{0, 0}}, {AccessPattern::CONTIGUOUS, PatternStatistics{0, 0}},
		{AccessPattern::STRIDED, PatternStatistics{93, 87}}, {AccessPattern::RANDOM, PatternStatistics{0, 0}} };
	EXPECT_EQ(result.pattern_per_timeinterval, STRIDED_SHOULD_PATTERN);
	EXPECT_EQ(result.stats_per_pattern, STRIDED_SHOULD_STATS);

	IOAccesses strided_only2 {
		IoAccess {0, 3, 0, 5},
		IoAccess {8, 30, 40, 1},
		IoAccess {31, 33, 80, 23},
		IoAccess {100, 130, 120, 5},
		IoAccess {131, 132, 160, 10},
	};
	result = access_pattern_detection::detect_local_access_pattern(strided_only2);
	std::unordered_map<TimeInterval, access_pattern_detection::AccessPattern, access_pattern_detection::pair_hash> STRIDED_SHOULD2 = { {std::pair(0, 132), access_pattern_detection::AccessPattern::STRIDED} };
	EXPECT_EQ(result.pattern_per_timeinterval, STRIDED_SHOULD2);

	// primes for fpos&sizes should be random enough
	IOAccesses random_only{
		IoAccess {0, 3, 1, 5, 3, false},
		IoAccess {8, 30, 5, 1, 7, false},
		IoAccess {31, 33, 11, 67, 3 ,false},
		IoAccess {100, 130, 17, 5, 14, false},
		IoAccess {131, 132, 23, 10, 27, false},
		IoAccess {132, 135, 31, 5, 33, false},
	};
	result = access_pattern_detection::detect_local_access_pattern(random_only);
	std::unordered_map<TimeInterval, access_pattern_detection::AccessPattern, access_pattern_detection::pair_hash> RANDOM_SHOULD = { {std::pair(0, 135), access_pattern_detection::AccessPattern::RANDOM} };
	std::unordered_map<AccessPattern, PatternStatistics> RANDOM_SHOULD_STATS = { {AccessPattern::NONE, PatternStatistics{0, 0}}, {AccessPattern::CONTIGUOUS, PatternStatistics{0, 0}},
		{AccessPattern::STRIDED, PatternStatistics{0, 0}}, {AccessPattern::RANDOM, PatternStatistics{93, 87}} };
	EXPECT_EQ(result.pattern_per_timeinterval, RANDOM_SHOULD);
	EXPECT_EQ(result.stats_per_pattern, RANDOM_SHOULD_STATS);
}

TEST(AccessPattern, Combined) {
	IOAccesses contiguous_and_strided {
		// STRIDED
		IoAccess {0, 3, 1000, 5, 3, false},
		IoAccess {8, 30, 2000, 1, 7, false},
		IoAccess {31, 33, 3000, 67, 3, false},
		IoAccess {100, 130, 4000, 5, 14, false},
		IoAccess {131, 132, 5000, 10, 27, false},
		IoAccess {132, 135, 6000, 5, 33, false},

		// CONTIGUOUS
		IoAccess {137, 139, 0, 5, 3, false},
		IoAccess {140, 141, 5, 1, 7, false},
		IoAccess {144, 146, 6, 67, 3, false},
		IoAccess {147, 148, 73, 5, 14, false},
		IoAccess {150, 151, 78, 10, 27, false},
		IoAccess {162, 185, 88, 5, 35, false},
	};
	auto result = access_pattern_detection::detect_local_access_pattern(contiguous_and_strided);
	std::unordered_map<TimeInterval, access_pattern_detection::AccessPattern, access_pattern_detection::pair_hash> CONTIGUOUS_AND_STRIDED =
	{ {std::pair(0, 135), access_pattern_detection::AccessPattern::STRIDED},
	  {std::pair(137, 185), access_pattern_detection::AccessPattern::CONTIGUOUS}
	};
	std::unordered_map<AccessPattern, PatternStatistics> COMBINED_SHOULD_STATS = { {AccessPattern::NONE, PatternStatistics{0, 0}}, {AccessPattern::CONTIGUOUS, PatternStatistics{93, 89}},
		{AccessPattern::STRIDED, PatternStatistics{93, 87}}, {AccessPattern::RANDOM, PatternStatistics{0, 0}} };
	EXPECT_EQ(result.pattern_per_timeinterval, CONTIGUOUS_AND_STRIDED);
	EXPECT_EQ(result.stats_per_pattern, COMBINED_SHOULD_STATS);
}
