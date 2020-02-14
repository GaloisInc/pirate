cmake_minimum_required(VERSION 3.13)

project(channel_demo)
set(VERSION 0.0.1)

set(CMAKE_BUILD_TYPE RelwithDebInfo)

# Dependencies
set(LIBPIRATE pirateprims-static)
set(LIBPIRATE_DIR ../../libpirate)
include_directories(${LIBPIRATE_DIR})

# Build flags
SET(BUILD_FLAGS "-Werror" "-Wall" "-Wextra" "-Wpedantic" "-O0" "-g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDEMO_VERSION='\"${VERSION}\"'")

# Common
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(COMMON_SRC ${SRC_DIR}/common.c)

function(add_channel_test_target TGT)
    add_executable(${TGT} ${COMMON_SRC} ${SRC_DIR}/${TGT}.c)
    target_link_libraries(${TGT} ${LIBPIRATE})
    target_compile_options(${TGT} PRIVATE ${BUILD_FLAGS})
endfunction()

# Targets
add_channel_test_target(reader)
add_channel_test_target(writer)
