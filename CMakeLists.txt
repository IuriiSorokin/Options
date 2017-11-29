cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project( options )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall")
SET(CMAKE_BUILD_TYPE Debug)

find_package( Boost REQUIRED program_options unit_test_framework )
include_directories( ${Boost_INCLUDE_DIRS} )

file(GLOB example_cpps "*.cpp")
foreach( example_cpp ${example_cpps} )
    get_filename_component(example ${example_cpp} NAME_WE )
    add_executable( ${example} ${example_cpp} )
    target_link_libraries( ${example} ${Boost_LIBRARIES}  )
endforeach( example_cpp ${example_cpps} )

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/run_examples )
    execute_process(COMMAND ln -s ${CMAKE_CURRENT_SOURCE_DIR}/run_examples ${CMAKE_CURRENT_BINARY_DIR}/ )
    execute_process(COMMAND chmod a+x ${CMAKE_CURRENT_BINARY_DIR}/run_examples )
endif()





