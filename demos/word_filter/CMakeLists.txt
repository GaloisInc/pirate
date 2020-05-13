cmake_minimum_required(VERSION 3.5)

project(word_filter)
set(CMAKE_BUILD_TYPE RelwithDebInfo)

# Sensitivity levels and output binary names
set(HIGH "filter_host")
set(LOW "filter_ui")

# Output directories
set(HIGH_DIR ${PROJECT_BINARY_DIR}/${HIGH})
set(LOW_DIR ${PROJECT_BINARY_DIR}/${LOW})
file(MAKE_DIRECTORY ${HIGH_DIR} ${LOW_DIR})

# Build flags
set(BUILD_FLAGS "-Werror" "-Wall" "-Wextra" "-Wpedantic" "-O0")
set(BUILD_FLAGS ${BUILD_FLAGS} "-ffunction-sections" "-fdata-sections" "--target=x86_64-pc-linux-elf")
#set(BUILD_FLAGS ${BUILD_FLAGS} "SHELL:-Xclang -load" "SHELL:-Xclang PropagateEnclaves.so")

# Source files
SET(SRCS "filter.c")

function(add_gaps_target TGT OUT_DIR)
    add_executable(${TGT} ${SRCS})
    target_link_libraries(${TGT} ${PIRATE_APP_LIBS} pthread)
    target_compile_options(${TGT} PRIVATE ${BUILD_FLAGS})
    set_target_properties(${TGT} PROPERTIES LINK_FLAGS "${LINK_FLAGS} -Wl,-enclave,${TGT} -fuse-ld=lld")
    set_target_properties(${TGT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})
endfunction()

add_gaps_target(${HIGH} ${HIGH_DIR})
add_gaps_target(${LOW} ${LOW_DIR})
