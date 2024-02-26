cmake_minimum_required(VERSION 3.13)

ADD_DEFINITIONS(-I../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)

IF (BUILD_EXAMPLES)
	ADD_EXECUTABLE(async_server server.c)
	TARGET_LINK_LIBRARIES(async_server ubus ${ubox_library} ${blob_library} ${json})
ENDIF()
