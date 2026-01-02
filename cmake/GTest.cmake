enable_testing()

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
)

# For Windows: disable installing gtest
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(my_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/unittests/detect_local_access_pattern.cpp)

add_test(
	NAME unittests
    COMMAND my_tests
)

target_link_libraries(my_tests
    GTest::gtest
    GTest::gtest_main
	otf-profiler-lib
)
target_include_directories(my_tests PRIVATE
	${PROJECT_SOURCE_DIR}/include
	${OTF2_PREFIX}/include
	${Boost_INCLUDE_DIRS}
)

include(GoogleTest)
gtest_discover_tests(my_tests)
