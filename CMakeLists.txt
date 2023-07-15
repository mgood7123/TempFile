cmake_minimum_required(VERSION 3.4.3)
project(tmpfile)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "")

if(NOT COMMAND testBuilder_build)
    add_subdirectory(testBuilder)
    testBuilder_set_current_working_directory_to_default_binary_directory()
endif()

testBuilder_add_source(tmpfile src/tmpfile.cpp)
testBuilder_add_source(tmpfile randombytes/randombytes.c)
testBuilder_add_include(tmpfile include)
testBuilder_build_shared_library(tmpfile)

