#include <gtest/gtest.h>
#include <otf2/OTF2_GeneralDefinitions.h>
#include <utility>
#include "access_pattern_detection.h"

TEST(AccessPattern, Simple) {

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

	IOAccesses strided_only {
		IoAccess {0, 3, 1000, 5},
		IoAccess {8, 30, 2000, 1},
		IoAccess {31, 33, 3000, 67},
		IoAccess {100, 130, 4000, 5},
		IoAccess {131, 132, 5000, 10},
		IoAccess {132, 135, 6000, 5},
	};
	result = access_pattern_detection::detect_local_access_pattern(strided_only);
	std::unordered_map<TimeInterval, access_pattern_detection::AccessPattern, access_pattern_detection::pair_hash> STRIDED_SHOULD = { {std::pair(0, 135), access_pattern_detection::AccessPattern::STRIDED} };
	EXPECT_EQ(result.pattern_per_timeinterval, STRIDED_SHOULD);

	// primes for fpos&sizes should be random enough
	IOAccesses random_only{
		IoAccess {0, 3, 1, 3},
		IoAccess {8, 30, 5, 7},
		IoAccess {31, 33, 11, 13},
		IoAccess {100, 130, 17, 19},
		IoAccess {131, 132, 23, 29},
		IoAccess {132, 135, 31, 37},
	};
	result = access_pattern_detection::detect_local_access_pattern(random_only);
	std::unordered_map<TimeInterval, access_pattern_detection::AccessPattern, access_pattern_detection::pair_hash> RANDOM_SHOULD = { {std::pair(0, 135), access_pattern_detection::AccessPattern::RANDOM} };
	EXPECT_EQ(result.pattern_per_timeinterval, RANDOM_SHOULD);
}

TEST(AccessPattern, Combined) {
	IOAccesses contiguous_and_strided {
		// STRIDED
		IoAccess {0, 3, 1000, 5},
		IoAccess {8, 30, 2000, 1},
		IoAccess {31, 33, 3000, 67},
		IoAccess {100, 130, 4000, 5},
		IoAccess {131, 132, 5000, 10},
		IoAccess {132, 135, 6000, 5},

		// CONTIGUOUS
		IoAccess {137, 139, 0, 5},
		IoAccess {140, 141, 5, 1},
		IoAccess {144, 146, 6, 67},
		IoAccess {147, 148, 73, 5},
		IoAccess {150, 151, 78, 10},
		IoAccess {162, 185, 88, 5},
	};
	auto result = access_pattern_detection::detect_local_access_pattern(contiguous_and_strided);
	std::unordered_map<TimeInterval, access_pattern_detection::AccessPattern, access_pattern_detection::pair_hash> CONTIGUOUS_AND_STRIDED =
	{ {std::pair(0, 135), access_pattern_detection::AccessPattern::STRIDED},
	  {std::pair(137, 185), access_pattern_detection::AccessPattern::CONTIGUOUS}
	};
	EXPECT_EQ(result.pattern_per_timeinterval, CONTIGUOUS_AND_STRIDED);
}
