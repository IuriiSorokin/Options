cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project( Options )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall")
SET(CMAKE_BUILD_TYPE Debug)

find_package( Boost REQUIRED program_options filesystem unit_test_framework )
include_directories( ${Boost_INCLUDE_DIRS} )
include_directories( "${PROJECT_SOURCE_DIR}" )

list( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake )
include(add_executables_glob_sources)

add_subdirectory(Examples)
add_subdirectory(Tests)





