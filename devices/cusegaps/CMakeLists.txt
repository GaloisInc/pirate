cmake_minimum_required(VERSION 3.5)
project(cusegaps LANGUAGES C)

set(CMAKE_BUILD_TYPE RelwithDebInfo)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")
find_package(FUSE REQUIRED)

include_directories(${FUSE_INCLUDE_DIR} ../../include)

add_executable(cusegaps_net cusegaps_net.c)
target_link_libraries(cusegaps_net ${FUSE_LIBRARIES})

add_executable(cusegaps_pipe cusegaps_pipe.c)
target_link_libraries(cusegaps_pipe ${FUSE_LIBRARIES})

add_executable(cusegaps_test cusegaps_test.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")
target_compile_options(cusegaps_net PRIVATE -Werror -Wall -Wextra -Wpedantic)
target_compile_options(cusegaps_pipe PRIVATE -Werror -Wall -Wextra -Wpedantic)
target_compile_options(cusegaps_test PRIVATE -Werror -Wall -Wextra -Wpedantic)
