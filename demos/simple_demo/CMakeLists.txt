cmake_minimum_required(VERSION 3.5)

project(simple_demo)
set(CMAKE_BUILD_TYPE RelwithDebInfo)

# Options
if (GAPS_DISABLE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGAPS_DISABLE")
endif(GAPS_DISABLE)

# Sensitivity levels and output binary names
set(HIGH "high")
set(LOW "low")

# Output directories
set(HIGH_DIR ${PROJECT_BINARY_DIR}/${HIGH})
set(LOW_DIR ${PROJECT_BINARY_DIR}/${LOW})
file(MAKE_DIRECTORY ${HIGH_DIR} ${LOW_DIR})

# Resources
set(RES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/res)
set(STATIC_HTML "index.html")
set(FAV_ICON "favicon.ico")
set(FAV_ICON_HIGH "favicon_high.ico")
set(FAV_ICON_LOW "favicon_low.ico")
configure_file(${RES_DIR}/${STATIC_HTML} ${HIGH_DIR} COPYONLY)
configure_file(${RES_DIR}/${FAV_ICON_HIGH} ${HIGH_DIR}/${FAV_ICON} COPYONLY)
configure_file(${RES_DIR}/${FAV_ICON_LOW} ${LOW_DIR}/${FAV_ICON} COPYONLY)

# Build flags
set(BUILD_FLAGS "-Werror" "-Wall" "-Wextra" "-Wpedantic" "-O3")
if (NOT GAPS_DISABLE)
    set(BUILD_FLAGS ${BUILD_FLAGS} "-ffunction-sections" "-fdata-sections" "--target=x86_64-pc-linux-elf")
endif(NOT GAPS_DISABLE)

# Source files
SET(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
SET(SRCS "${SRC_DIR}/demo.c" "${SRC_DIR}/tiny.c")

function(add_gaps_target TGT OUT_DIR)
    add_executable(${TGT} ${SRCS})
    target_link_libraries(${TGT} ${PIRATE_APP_LIBS} pthread)
    target_compile_options(${TGT} PRIVATE ${BUILD_FLAGS})
    set_target_properties(${TGT} PROPERTIES LINK_FLAGS "${LINK_FLAGS} -Wl,-enclave,${TGT} -fuse-ld=lld")
    set_target_properties(${TGT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})
endfunction()

function(add_non_gaps_target TGT OUT_DIR)
    string(TOUPPER ${TGT} TGT_FLAG)
    add_executable(${TGT} ${SRCS})
    target_link_libraries(${TGT} ${PIRATE_APP_LIBS} pthread)
    target_compile_options(${TGT} PRIVATE ${BUILD_FLAGS} "-D${TGT_FLAG}")
    set_target_properties(${TGT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})
endfunction()

if (GAPS_DISABLE)
    add_non_gaps_target(${HIGH} ${HIGH_DIR})
    add_non_gaps_target(${LOW} ${LOW_DIR})
else()
    add_gaps_target(${HIGH} ${HIGH_DIR})
    add_gaps_target(${LOW} ${LOW_DIR})
endif(GAPS_DISABLE)

# Combined binary
if (SINGLE_BINARY)
    set(DEMO demo)
    set(DEMO_HIGH "${HIGH_DIR}/${DEMO}")
    set(DEMO_LOW "${LOW_DIR}/${DEMO}")
    set(DEMO_OUT ${DEMO_HIGH} ${DEMO_LOW})
    set(DEVICE_FILE .dev_id)
    set(PLATFORM_GAPS_TOOL_DIR ${PROJECT_SOURCE_DIR}/../../single-binary)
    set(PLATFORM_GAPS_TOOL ${PLATFORM_GAPS_TOOL_DIR}/platform.py)

    add_custom_command(
        OUTPUT ${DEMO_OUT}
        COMMAND ${PLATFORM_GAPS_TOOL} ${DEMO_HIGH} ${DEVICE_FILE} ${HIGH_DIR}/${HIGH} ${HIGH} ${LOW_DIR}/${LOW} ${LOW}
        COMMAND cp -p ${DEMO_HIGH} ${DEMO_LOW}
        COMMAND echo "${HIGH}" > ${HIGH_DIR}/${DEVICE_FILE}
        COMMAND echo "${LOW}" > ${LOW_DIR}/${DEVICE_FILE}
        WORKING_DIRECTORY ${PLATFORM_GAPS_TOOL_DIR}
        COMMENT "Generating combined application binary"
        DEPENDS ${HIGH} ${LOW}
        VERBATIM
    )
    add_custom_target(DEMO_TGT ALL DEPENDS ${DEMO_OUT})
endif (SINGLE_BINARY)

# Test
set(TEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)
set(TEST_OUT_DIR ${PROJECT_BINARY_DIR}/test)
file(MAKE_DIRECTORY ${TEST_OUT_DIR})
configure_file(${TEST_SRC_DIR}/index.filtered.html ${TEST_OUT_DIR} COPYONLY)
configure_file(${TEST_SRC_DIR}/integration_test.py ${TEST_OUT_DIR} COPYONLY)
