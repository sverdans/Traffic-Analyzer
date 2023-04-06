cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

set(PROJECT_NAME traffic-analyzer)

#find_package(pcapplusplus REQUIRED)

project(${PROJECT_NAME})

add_executable(${PROJECT_NAME}
	source/main.cpp
	source/AppHandler.h
	source/AppHandler.cpp
	source/HttpStatsCollector.h
)

#target_include_directories(${PROJECT_NAME} ${PCAPPLUSPLUS_INCLUDE_DIRS})
#target_link_libraries(${PROJECT_NAME} ${PCAPPLUSPLUS_LIBRARIES})

target_link_libraries(${PROJECT_NAME}
	Pcap++ 
	Packet++ 
	Common++ 
	pcap
)

target_include_directories(${PROJECT_NAME} PUBLIC
	/usr/local/include/pcapplusplus
)

