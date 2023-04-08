cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE ON)

project(traffic-analyzer)

if(NOT PKG_CONFIG_FOUND)
	include(CMakeFindDependencyMacro)
	find_dependency(PkgConfig)
endif()

include(FetchContent)

# FetchContent_Declare(pcapplusplus
# GIT_REPOSITORY https://github.com/seladb/PcapPlusPlus
# SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/PcapPlusPlus
# GIT_TAG master
# )

# FetchContent_GetProperties(pcapplusplus)
# if(NOT pcapplusplus_POPULATED)
# FetchContent_Populate(pcapplusplus)
# add_subdirectory(${pcapplusplus_SOURCE_DIR} ${pcapplusplus_BINARY_DIR})
# endif()
find_package(Boost REQUIRED COMPONENTS
	system
	log
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
	Boost::program_options
	
	Pcap++
	Packet++
	Common++
	pcap
)

target_include_directories(traffic-analyzer PUBLIC
	/usr/local/include/pcapplusplus
)

# target_link_libraries(traffic-analyzer PRIVATE ${PCAPPLUSPLUS_INCLUDE_DIRS})
# target_include_directories(traffic-analyzer PRIVATE ${PCAPPLUSPLUS_LIBRARIES})