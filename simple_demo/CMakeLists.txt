cmake_minimum_required(VERSION 3.5)

project(simple_demo)

include_directories(../libpirate ../tiny)

SET(CMAKE_C_COMPILER "clang")

add_executable(high high.c)
target_link_libraries(high pirateprims-static libtiny pthread)
target_compile_options(high PRIVATE -Werror -Wall -Wextra -Wpedantic)

add_executable(low low.c)
target_link_libraries(low pirateprims-static libtiny)
target_compile_options(high PRIVATE -Werror -Wall -Wextra -Wpedantic)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/index.html ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
