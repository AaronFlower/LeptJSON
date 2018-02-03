# CMake 最低版本要求
cmake_minimum_required (VERSION 3.10)

if (CMAKE_C_COMPILED_ID MATCHES "GUN|Clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -pedantic -Wall")
endif()

add_library(leptjson leptjson.c)
add_executable(leptjson_test test.c)
target_link_libraries(leptjson_test leptjson)
