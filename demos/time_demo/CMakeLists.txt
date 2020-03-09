cmake_minimum_required(VERSION 3.13)

project(time_demo)
set(CMAKE_BUILD_TYPE RelwithDebInfo)

# Build version
set(VERSION 0.0.2)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDEMO_VERSION='\"${VERSION}\"'")

# Options
option(GAPS_SERIAL "Use serial devices for proxy <-> signer GAPS channels" OFF)

if(GAPS_SERIAL)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGAPS_SERIAL")
endif(GAPS_SERIAL)

if (GAPS_ENABLE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGAPS_ENABLE")
endif(GAPS_ENABLE)

# Dependencies
find_package(OpenSSL REQUIRED)
find_library(JPEG_LIB jpeg)
if(NOT JPEG_LIB)
  message(FATAL_ERROR "jpeg library not found")
endif()
find_library(X11_LIB X11)
if(NOT X11_LIB)
  message(FATAL_ERROR "X11 library not found")
endif()
set(CRYPTO_KIT openssl)
include_directories(./include ${OPENSSL_INCLUDE_DIR})
set(COMMON_LIB ${OPENSSL_LIBRARIES} ${PIRATE_APP_LIBS} "X11" "jpeg")
set(SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/scripts)
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)

# Sensitivity levels
set(LEVEL_ORANGE "orange")
set(LEVEL_YELLOW "yellow")
set(LEVEL_PURPLE "purple")

# Output binaries
set(SENSOR_MANAGER sensor_manager)
set(SIGNING_PROXY signing_proxy)
set(SIGNING_SERVICE signing_service)

# Output directories
set(ORANGE_DIR ${PROJECT_BINARY_DIR}/${LEVEL_ORANGE})
set(YELLOW_DIR ${PROJECT_BINARY_DIR}/${LEVEL_YELLOW})
set(PURPLE_DIR ${PROJECT_BINARY_DIR}/${LEVEL_PURPLE})
file(MAKE_DIRECTORY ${ORANGE_DIR} ${YELLOW_DIR} ${PURPLE_DIR})

# Build flags
set(BUILD_FLAGS "-Werror" "-Wall" "-Wextra" "-Wpedantic" "-O3")
if (GAPS_ENABLE)
    set(BUILD_FLAGS ${BUILD_FLAGS} "-ffunction-sections" "-fdata-sections" "--target=x86_64-pc-linux-elf")
endif(GAPS_ENABLE)

# Source files
set(COMMON_SRC
    "${SRC_DIR}/gaps_packet.c"
    "${SRC_DIR}/common.c"
    "${SRC_DIR}/ts_crypto.c"
    "${SRC_DIR}/video_sensor.c"
    "${SRC_DIR}/xwin_display.c"
)

set(TGT_SRC
    "${SRC_DIR}/${SENSOR_MANAGER}.c"
    "${SRC_DIR}/${SIGNING_PROXY}.c"
    "${SRC_DIR}/${SIGNING_SERVICE}.c"
)
set(ALL_SRC ${COMMON_SRC} ${TGT_SRC})

function(add_gaps_target TGT SRC LVL)
    add_executable(${TGT} ${SRC})
    target_link_libraries(${TGT} ${COMMON_LIB})
    target_compile_options(${TGT} PRIVATE ${BUILD_FLAGS})
    target_link_options(${TGT} PRIVATE "LINKER:-enclave" "LINKER:${LVL}" "-fuse-ld=lld")
    set_target_properties(
        ${TGT}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY
        "${PROJECT_BINARY_DIR}/${LVL}"
    )
endfunction()

function(add_non_gaps_target TGT TGT_DIR)
    set(TGT_SRC "${PROJECT_SOURCE_DIR}/src/${TGT}.c")
    add_executable(${TGT} ${TGT_SRC} ${COMMON_SRC})
    target_link_libraries(${TGT} ${COMMON_LIB})
    target_compile_options(${TGT} PRIVATE ${BUILD_FLAGS})
    set_target_properties(${TGT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${TGT_DIR})
endfunction()

if (GAPS_ENABLE)
    add_gaps_target(${SENSOR_MANAGER}  "${ALL_SRC}" ${LEVEL_ORANGE})
    add_gaps_target(${SIGNING_PROXY}   "${ALL_SRC}" ${LEVEL_YELLOW})
    add_gaps_target(${SIGNING_SERVICE} "${ALL_SRC}" ${LEVEL_PURPLE})
else()
    add_non_gaps_target(${SENSOR_MANAGER}  ${ORANGE_DIR})
    add_non_gaps_target(${SIGNING_PROXY}   ${YELLOW_DIR})
    add_non_gaps_target(${SIGNING_SERVICE} ${PURPLE_DIR})
endif(GAPS_ENABLE)

# TSA directories
set(CA_DIR ${PROJECT_BINARY_DIR}/ca)
set(CA_PRIV_DIR ${CA_DIR}/.priv)
set(TSA_DIR ${PROJECT_BINARY_DIR}/tsa)
set(TSA_PRIV_DIR ${PURPLE_DIR}/.priv)
file(MAKE_DIRECTORY ${CA_DIR} ${CA_PRIV_DIR} ${TSA_DIR} ${TSA_PRIV_DIR})

# TSA configuration resources
set(CONF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/conf)
set(CONF_CA ${CONF_DIR}/tsa_ca.conf)
set(CONF_TSA ${CONF_DIR}/tsa.conf)
configure_file(${CONF_TSA} ${TSA_PRIV_DIR} COPYONLY)

# Timestamp verification script
set(TS_VERIFY_SCRIPT ${SCRIPTS_DIR}/ts_verify.sh)
configure_file(${TS_VERIFY_SCRIPT} ${ORANGE_DIR} COPYONLY)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/stock DESTINATION ${ORANGE_DIR})

# Certificate Authority
set(TSA_CA_CERT ${TSA_DIR}/tsa_ca.pem)
set(TSA_CA_KEY ${CA_PRIV_DIR}/tsa_ca_key.pem)
set(TSA_CA_OUT ${TSA_CA_CERT} ${TSA_CA_KEY})

add_custom_command(
    OUTPUT ${TSA_CA_OUT}
    COMMAND ${CRYPTO_KIT} req -new -x509 -nodes -config ${CONF_CA} -out ${TSA_CA_CERT} -keyout ${TSA_CA_KEY}
    WORKING_DIRECTORY ${TSA_CA_DIR}
    COMMENT "Generating Certificate Authority"
    DEPENDS ${CONF_CA}
    VERBATIM
)
add_custom_target(TSA_CA_TGT ALL DEPENDS ${TSA_CA_OUT})

# Timestamp Authority
set(TSA_REQ ${TSA_PRIV_DIR}/tsa_req.pem)
set(TSA_KEY ${TSA_PRIV_DIR}/tsa_key.pem)
set(TSA_CERT ${TSA_DIR}/tsa_cert.pem)
set(TSA_OUT ${TSA_REQ} ${TSA_KEY} ${TSA_CERT})

add_custom_command(
    OUTPUT ${TSA_OUT}
    COMMAND DN_SECTION="tsa" ${CRYPTO_KIT} req -new -config ${CONF_CA} -out ${TSA_REQ} -keyout ${TSA_KEY}
    COMMAND DN_SECTION="tsa" ${CRYPTO_KIT} x509 -req -in ${TSA_REQ} -out ${TSA_CERT} -CA ${TSA_CA_CERT} -CAkey ${TSA_CA_KEY} -CAcreateserial -extfile ${CONF_CA} -extensions tsa_cert
    WORKING_DIRECTORY ${TSA_CA_DIR}
    COMMENT "Generating Timestamp Authority Certificate"
    DEPENDS ${TSA_CA_TGT} ${TSA_CA_OUT}
    VERBATIM
)
add_custom_target(TSA_TGT ALL DEPENDS ${TSA_OUT} ${TSA_CA_OUT})

# Combined timestamp binary
if (SINGLE_BINARY)
    set(TS_DEMO ts_demo)
    set(TS_DEMO_ORANGE "${ORANGE_DIR}/${TS_DEMO}")
    set(TS_DEMO_YELLOW "${YELLOW_DIR}/${TS_DEMO}")
    set(TS_DEMO_PURPLE "${PURPLE_DIR}/${TS_DEMO}")
    set(TS_DEMO_OUT ${TS_DEMO_ORANGE} ${TS_DEMO_YELLOW} ${TS_DEMO_PURPLE})
    set(DEVICE_FILE .dev_id)
    set(PLATFORM_GAPS_TOOL_DIR ${PROJECT_SOURCE_DIR}/../../single-binary)
    set(PLATFORM_GAPS_TOOL ${PLATFORM_GAPS_TOOL_DIR}/platform.py)

    add_custom_command(
        OUTPUT ${TS_DEMO_OUT}
        COMMAND ${PLATFORM_GAPS_TOOL} ${TS_DEMO_ORANGE} ${DEVICE_FILE} ${ORANGE_DIR}/${SENSOR_MANAGER} ${SENSOR_MANAGER} ${YELLOW_DIR}/${SIGNING_PROXY} ${SIGNING_PROXY} ${PURPLE_DIR}/${SIGNING_SERVICE} ${SIGNING_SERVICE}
        COMMAND cp -p ${TS_DEMO_ORANGE} ${TS_DEMO_YELLOW}
        COMMAND cp -p ${TS_DEMO_ORANGE} ${TS_DEMO_PURPLE}
        COMMAND echo "${SENSOR_MANAGER}" > ${ORANGE_DIR}/${DEVICE_FILE}
        COMMAND echo "${SIGNING_PROXY}" > ${YELLOW_DIR}/${DEVICE_FILE}
        COMMAND echo "${SIGNING_SERVICE}" > ${PURPLE_DIR}/${DEVICE_FILE}
        WORKING_DIRECTORY ${PLATFORM_GAPS_TOOL_DIR}
        COMMENT "Generating combined application binary"
        DEPENDS ${SENSOR_MANAGER} ${SIGNING_PROXY} ${SIGNING_SERVICE}
        VERBATIM
    )
    add_custom_target(TS_DEMO_TGT ALL DEPENDS ${TS_DEMO_OUT})
endif (SINGLE_BINARY)

# Tests
set(TEST_DIR ${PROJECT_BINARY_DIR}/test)
file(MAKE_DIRECTORY ${TEST_DIR})

# Timestamp test
set(TS_TEST ts_test)
add_non_gaps_target(${TS_TEST} ${TEST_DIR})
set(TS_TEST_SCRIPT ${SCRIPTS_DIR}/ts_test.sh)
configure_file(${TS_TEST_SCRIPT} ${TEST_DIR} COPYONLY)

# UART test
if(GAPS_SERIAL)
    set(UART_TEST uart_test)
    add_non_gaps_target(${UART_TEST} ${TEST_DIR})
endif(GAPS_SERIAL)
