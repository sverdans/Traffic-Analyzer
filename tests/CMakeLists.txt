cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

enable_testing()

file(GLOB_RECURSE TEST_SOURCE "./*.h" "./*.cpp")

add_executable(mytest ${TEST_SOURCE})

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

	nlohmann_json::nlohmann_json)

add_test(mygtest mytest)
