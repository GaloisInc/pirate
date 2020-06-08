cmake_minimum_required(VERSION 3.5)

project(word_filter_with_pal)
set(CMAKE_BUILD_TYPE RelwithDebInfo)

# Sensitivity levels and output binary names
set(HIGH "filter_host")
set(LOW "filter_ui")

# Build flags
set(BUILD_FLAGS "-Werror" "-Wall" "-Wextra" "-Wpedantic" "-O0")
set(BUILD_FLAGS ${BUILD_FLAGS} "-ffunction-sections" "-fdata-sections" "--target=x86_64-pc-linux-elf")
# set(BUILD_FLAGS ${BUILD_FLAGS} "SHELL:-Xclang -load" "SHELL:-Xclang PropagateEnclaves.so")

# Source files
SET(SRCS "filter.c")

function(add_gaps_target TGT)
    add_executable(${TGT}_pal ${SRCS})
    target_link_libraries(${TGT}_pal ${PIRATE_APP_LIBS} libpal_shared)
    target_compile_options(${TGT}_pal PRIVATE ${BUILD_FLAGS})
    set_target_properties(${TGT}_pal PROPERTIES LINK_FLAGS "${LINK_FLAGS} -Wl,-enclave,${TGT} -fuse-ld=lld")
endfunction()

add_gaps_target(${HIGH})
add_gaps_target(${LOW})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/word_filter_with_pal.yaml
    DESTINATION ${PROJECT_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
