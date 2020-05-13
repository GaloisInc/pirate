cmake_minimum_required(VERSION 3.5)

project(channel_demo)
set(VERSION 0.0.2)

set(CMAKE_BUILD_TYPE RelwithDebInfo)

# Build flags
SET(BUILD_FLAGS ${PIRATE_C_FLAGS} "-O0" "-g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDEMO_VERSION='\"${VERSION}\"'")

# Common
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(COMMON_SRC ${SRC_DIR}/common.c)

function(add_channel_test_target TGT)
    add_executable(${TGT} ${COMMON_SRC} ${SRC_DIR}/${TGT}.c)
    target_link_libraries(${TGT} ${PIRATE_APP_LIBS})
    target_compile_options(${TGT} PRIVATE ${BUILD_FLAGS})
endfunction()

# Targets
add_channel_test_target(reader)
add_channel_test_target(writer)

configure_file(src/perf.py ${PROJECT_BINARY_DIR} COPYONLY)
