cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(PROJECT_NAME traffic-analyzer)

project(${PROJECT_NAME})

if(NOT PKG_CONFIG_FOUND)
    include(CMakeFindDependencyMacro)
    find_dependency(PkgConfig)
endif()

function(fetch_dependencies)

    include(FetchContent)

    set(LIBS_DIR ${CMAKE_BINARY_DIR}/external)

    message(STATUS "Fetching Deps started")

    set(PCAPPP_BUILD_EXAMPLES OFF)
    set(PCAPPP_BUILD_TESTS OFF)
    FetchContent_Declare(PcapPlusPlus
        GIT_REPOSITORY https://github.com/seladb/PcapPlusPlus.git
        SOURCE_DIR ${LIBS_DIR}/PcapPlusPlus
        GIT_TAG master)
    FetchContent_MakeAvailable(PcapPlusPlus)

    set(SERVED_BUILD_EXAMPLES OFF)
    FetchContent_Declare(served
        GIT_REPOSITORY https://github.com/meltwater/served.git
        SOURCE_DIR ${LIBS_DIR}/served
        GIT_TAG master)
    FetchContent_MakeAvailable(served)
    target_include_directories(served PUBLIC ${LIBS_DIR}/served/src)

    set(JSON_BuildTests OFF CACHE INTERNAL "")
    FetchContent_Declare(json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        SOURCE_DIR ${LIBS_DIR}/nlohmann
        GIT_TAG v3.11.2)
    FetchContent_MakeAvailable(json)

    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        SOURCE_DIR ${LIBS_DIR}/google/googletest
        GIT_TAG main)
    FetchContent_MakeAvailable(googletest)

    message(STATUS "Fetching Deps done")

    find_package(Boost REQUIRED COMPONENTS
        system
        log
        log_setup
        program_options)

endfunction()

fetch_dependencies()

file(GLOB_RECURSE SOURCE "source/*.h" "source/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCE})

target_include_directories(${PROJECT_NAME} PUBLIC ${LIBS_DIR}/served/src)
    
include_directories(${CMAKE_SOURCE_DIR}/source)

target_link_libraries(${PROJECT_NAME} PRIVATE
    served
    Pcap++
    Packet++
    Common++
    nlohmann_json::nlohmann_json
    Boost::log
    Boost::log_setup
    Boost::program_options
    Boost::system)

add_subdirectory(tests)
add_subdirectory(docs)
