# Author: AaronFlower

# A simple example CMake Setup for compiling C Programs. This setup utilizes a single# subdirectory (src) in which all C source(.c) and header (.h) files within that
# directory are included within the project.

# CMake 最低版本要求
cmake_minimum_required (VERSION 3.10)

# project name
project (leptJson)

# Add a subdirectory to the project. 
# The CMakeLists.txt file in that subdirectory will
# be used for further setting up the project.

add_subdirectory( src )
