cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE ON)
project(traffic-analyzer)

if(NOT PKG_CONFIG_FOUND)
	include(CMakeFindDependencyMacro)
	find_dependency(PkgConfig)
endif()

include(FetchContent)

FetchContent_Declare(json 
	URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/nlohmann_json)
FetchContent_MakeAvailable(json)

FetchContent_Declare(googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/google/googletest
	GIT_TAG main)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(benchmark
	GIT_REPOSITORY https://github.com/google/benchmark.git
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/benchmark
	GIT_TAG main)
FetchContent_MakeAvailable(benchmark)

FetchContent_Declare(PcapPlusPlus
	GIT_REPOSITORY https://github.com/seladb/PcapPlusPlus.git
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/PcapPlusPlus
	GIT_TAG master)
FetchContent_MakeAvailable(PcapPlusPlus)

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

find_package(PkgConfig REQUIRED)
pkg_check_modules(SERVED_PKG REQUIRED IMPORTED_TARGET served)

target_include_directories(traffic-analyzer PRIVATE ${PCAPPLUSPLUS_HEADER_DIR})

target_link_libraries(traffic-analyzer PRIVATE
	Boost::system
	Boost::log
	Boost::log_setup
	Boost::program_options

	Pcap++
	Packet++
	Common++

	PkgConfig::SERVED_PKG

	nlohmann_json::nlohmann_json
	benchmark::benchmark
)
