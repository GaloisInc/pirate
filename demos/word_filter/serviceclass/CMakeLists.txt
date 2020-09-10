cmake_minimum_required(VERSION 3.5)

project(word_filter)
set(CMAKE_BUILD_TYPE RelwithDebInfo)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

# Sensitivity levels and output binary names
set(HIGH "filter_host_sc")
set(LOW "filter_ui_sc")

# Output directories
set(HIGH_DIR ${PROJECT_BINARY_DIR}/${HIGH})
set(LOW_DIR ${PROJECT_BINARY_DIR}/${LOW})
file(MAKE_DIRECTORY ${HIGH_DIR} ${LOW_DIR})

# Build flags
set(BUILD_FLAGS "-Wall" "-Wextra" "-Wpedantic" "-O2" "-Wno-zero-length-array")
set(BUILD_FLAGS ${BUILD_FLAGS} "-ffunction-sections" "-fdata-sections")

# Source files
SET(SRCS filter.cpp bidirservice.hpp fixedstring.hpp serialize.hpp request.hpp response.hpp)

function(add_gaps_target TGT OUT_DIR)
    add_executable(${TGT} ${SRCS})
    target_link_libraries(${TGT} ${PIRATE_APP_LIBS} pal)
    target_compile_options(${TGT} PRIVATE ${BUILD_FLAGS})
    set_target_properties(${TGT} PROPERTIES LINK_FLAGS "${LINK_FLAGS} -Wl,-enclave,${TGT} -fuse-ld=lld")
    set_target_properties(${TGT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})
endfunction()

add_gaps_target(${HIGH} ${HIGH_DIR})
add_gaps_target(${LOW} ${LOW_DIR})
