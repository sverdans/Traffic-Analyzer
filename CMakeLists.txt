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

FetchContent_Declare(PcapPlusPlus
	GIT_REPOSITORY https://github.com/seladb/PcapPlusPlus.git
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/PcapPlusPlus
	GIT_TAG master)
FetchContent_MakeAvailable(PcapPlusPlus)

#FetchContent_Declare(served
#	GIT_REPOSITORY https://github.com/meltwater/served.git
#	SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/served
#	GIT_TAG master)
#FetchContent_MakeAvailable(served)

find_package(Boost REQUIRED COMPONENTS
	system
	log
	log_setup
	program_options
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SERVED_PKG REQUIRED IMPORTED_TARGET served)

file(GLOB_RECURSE SOURCE
	"source/*.h"
	"source/*.cpp"
)

add_executable(traffic-analyzer ${SOURCE})

target_link_libraries(traffic-analyzer PRIVATE
	Boost::system
	Boost::log
	Boost::log_setup
	Boost::program_options

	Pcap++
	Packet++
	Common++

	#served
	PkgConfig::SERVED_PKG

	nlohmann_json::nlohmann_json
)

add_subdirectory(tests)
add_subdirectory(docs)