cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

enable_testing()

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
)

target_link_libraries(mytest PRIVATE 
	gtest_main
	gmock_main

	Pcap++
	Packet++
	Common++

	Boost::system
	Boost::log
	Boost::log_setup
	Boost::program_options

	nlohmann_json::nlohmann_json
)

add_test(mygtest mytest)