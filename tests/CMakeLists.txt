cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

FetchContent_Declare(googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/google/googletest
	GIT_TAG main)
FetchContent_MakeAvailable(googletest)

#add_library(GTest::GTest INTERFACE IMPORTED)
#target_link_libraries(GTest::GTest INTERFACE gtest_main)

file(GLOB_RECURSE TEST_SOURCE
	"./*.h"
	"./*.cpp"
)

add_executable(mytest 
	${TEST_SOURCE}
	../source/TrafficAnalyzer.h
)

target_link_libraries(mytest PRIVATE 
	gtest_main)
