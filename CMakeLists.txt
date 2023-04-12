cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE ON)

project(traffic-analyzer)

if(NOT PKG_CONFIG_FOUND)
	include(CMakeFindDependencyMacro)
	find_dependency(PkgConfig)
endif()

include(FetchContent)

#FetchContent_Declare(PcapPlusPlus
#	GIT_REPOSITORY https://github.com/seladb/PcapPlusPlus.git
#	SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/PcapPlusPlus
#	GIT_TAG master
#)
#FetchContent_MakeAvailable(PcapPlusPlus)

find_package(PcapPlusPlus REQUIRED)

find_package(Boost REQUIRED COMPONENTS
	system
	log
	log_setup
	program_options
)

file(GLOB_RECURSE SOURCE
	"source/*.h"
	"source/*.cpp"
)

add_executable(traffic-analyzer ${SOURCE})

target_link_libraries(traffic-analyzer
	Boost::system
	Boost::log
	Boost::log_setup
	Boost::program_options
	
	PcapPlusPlus::Pcap++
	PcapPlusPlus::Packet++
	PcapPlusPlus::Common++
)
