cmake_minimum_required(VERSION 3.5)

project(time_demo)
set(VERSION 0.0.1)

find_package(OpenSSL REQUIRED)

include_directories(./include ../libpirate ${OPENSSL_INCLUDE_DIR})

#add_library(openddl_dbg SHARED IMPORTED)
#set_property(TARGET openddl_dbg PROPERTY IMPORTED_LOCATION "../../openssl/build/lib/libcrypto.so.3")


set(CMAKE_C_COMPILER "clang")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDEMO_VERSION='\"${VERSION}\"' -g")
set(BUILD_FLAGS "-Werror" "-Wall" "-Wextra" "-Wpedantic")
set(PIRATE_LIB "pirateprims-static")
set(COMMON_SRC
    "${PROJECT_SOURCE_DIR}/src/gaps_packet.c"
    "${PROJECT_SOURCE_DIR}/src/common.c"
    "${PROJECT_SOURCE_DIR}/src/ts_crypto.c"
)

set(LEVEL_HIGH "high")
set(LEVEL_LOW "low")

function(add_gaps_target TGT LVL)
    set(TGT_SRC "${PROJECT_SOURCE_DIR}/src/${TGT}.c")
    add_executable(${TGT} ${TGT_SRC} ${COMMON_SRC})
    target_link_libraries(${TGT} ${PIRATE_LIB} ${OPENSSL_LIBRARIES} "pthread")
    target_compile_options(${TGT} PRIVATE ${BUILD_FLAGS})
    set_target_properties(
        ${TGT}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY
        "${PROJECT_BINARY_DIR}/${LVL}"
    )
endfunction()

# Build targets
set(PLANNER_NAME planner)
set(SIGNING_PROXY_NAME signing_proxy)
set(SIGNING_SERVICE_NAME signing_service)
add_gaps_target(${PLANNER_NAME} ${LEVEL_HIGH})
add_gaps_target(${SIGNING_PROXY_NAME} ${LEVEL_HIGH})
add_gaps_target(${SIGNING_SERVICE_NAME} ${LEVEL_LOW})

# Configuration resources
set(CONF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/conf)
set(CONF_CA ${CONF_DIR}/tsa_ca.conf)
set(CONF_TSA ${CONF_DIR}/tsa.conf)
set(PKI_DIR_LOW ${PROJECT_BINARY_DIR}/${LEVEL_LOW}/pki)
set(PKI_DIR_LOW_PRIV ${PKI_DIR_LOW}/priv)
file(MAKE_DIRECTORY 
    ${PKI_DIR_LOW}
    ${PKI_DIR_LOW_PRIV}
)
configure_file(${CONF_TSA} ${PKI_DIR_LOW} COPYONLY)

# Crypto tool
set(CRYPTO_KIT openssl)

# Certificate Authority
set(TSA_CA_DIR ${PROJECT_BINARY_DIR}/ca)
set(TSA_CA_PRIV_DIR ${TSA_CA_DIR}/priv)
file(MAKE_DIRECTORY ${TSA_CA_DIR} ${TSA_CA_PRIV_DIR})
set(TSA_CA ${TSA_CA_DIR}/tsa_ca.pem)
set(TSA_CA_KEY ${TSA_CA_PRIV_DIR}/tsa_ca_key.pem)
set(TSA_CA_OUT ${TSA_CA} ${TSA_CA_KEY})

add_custom_command(
    OUTPUT ${TSA_CA_OUT}
    COMMAND ${CRYPTO_KIT} req -new -x509 -nodes -config ${CONF_CA} -out ${TSA_CA} -keyout ${TSA_CA_KEY}
    WORKING_DIRECTORY ${TSA_CA_DIR}
    COMMENT "Generating Certificate Authority"
    DEPENDS ${CONF_CA}
    VERBATIM
)
add_custom_target(TSA_CA_TGT ALL DEPENDS ${TSA_CA_OUT})

# Timestamp Authority
set(TSA_REQ ${PKI_DIR_LOW_PRIV}/tsa_req.pem)
set(TSA_KEY ${PKI_DIR_LOW_PRIV}/tsa_key.pem)
set(TSA_CERT ${PKI_DIR_LOW}/tsa_cert.pem)
set(TSA_OUT ${TSA_REQ} ${TSA_KEY} ${TSA_CERT})

add_custom_command(
    OUTPUT ${TSA_OUT}
    COMMAND DN_SECTION="tsa" ${CRYPTO_KIT} req -new -config ${CONF_CA} -out ${TSA_REQ} -keyout ${TSA_KEY}
    COMMAND DN_SECTION="tsa" ${CRYPTO_KIT} x509 -req -in ${TSA_REQ} -out ${TSA_CERT} -CA ${TSA_CA} -CAkey ${TSA_CA_KEY} -CAcreateserial -extfile ${CONF_CA} -extensions tsa_cert
    WORKING_DIRECTORY ${TSA_CPKI_DIR_LOWA_DIR}
    COMMENT "Generating Timestamp Authority Certificate"
    DEPENDS ${TSA_CA_TGT} ${TSA_CA_OUT}
    VERBATIM
)
add_custom_target(TSA_TGT ALL DEPENDS ${TSA_OUT} ${TSA_CA_OUT})

# Test
set(SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/scripts)
set(TS_TEST_SCRIPT ${SCRIPTS_DIR}/ts_test.sh)
configure_file(${TS_TEST_SCRIPT} ${PROJECT_BINARY_DIR}/${LEVEL_LOW} COPYONLY)

set(TS_TEST_NAME ts_test)
add_gaps_target(${TS_TEST_NAME} ${LEVEL_LOW})
