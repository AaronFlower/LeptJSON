# Sets the compilation flags to report all warnings and enable debugging
# in the generated object files and executable.
set(CMAKE_C_FLAGS "-Wall -g -O0")

if (CMAKE_C_COMPILED_ID MATCHES "GUN|Clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -pedantic -Wall -g -O0")
endif()

# the following file GLOB command will search for all files within the 
# current directory that match the specified expressions (*.c and *.h), and
# assign the resulting files to the variables SRCS. 
# Thus, SRCS is a list a all C source and header in the current directory.

file (GLOB SRCS *.c *.h)

add_library(leptjson leptjson.c)
add_executable(leptjson_test ${SRCS})
target_link_libraries(leptjson_test leptjson)
