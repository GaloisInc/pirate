cmake_minimum_required(VERSION 3.5)
project(cusegaps LANGUAGES C)

set(CMAKE_BUILD_TYPE RelwithDebInfo)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")
find_package(FUSE REQUIRED)

include_directories(${FUSE_INCLUDE_DIR})
add_executable(cusegaps cusegaps.c)
target_link_libraries(cusegaps ${FUSE_LIBRARIES})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")
target_compile_options(cusegaps PRIVATE -Werror -Wall -Wextra -Wpedantic)
