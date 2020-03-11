cmake_minimum_required(VERSION 3.5)
project(enclave_demo)

SET(BUILD_FLAGS ${PIRATE_C_FLAGS})

function(add_gaps_target TGT SRC)
    add_executable(${TGT} ${SRC})
    target_compile_options(${TGT} PRIVATE ${BUILD_FLAGS})
    target_link_options(${TGT} PRIVATE "LINKER:-enclave" "LINKER:${TGT}" "-fuse-ld=lld")
endfunction()

if(GAPS_ENABLE)
    SET(ENCLAVE_SOURCES "enclave.c")
    add_gaps_target("alpha" ${ENCLAVE_SOURCES})
    add_gaps_target("beta" ${ENCLAVE_SOURCES})
    add_gaps_target("gamma" ${ENCLAVE_SOURCES})
else()
    message(STATUS "GAPS clang not present")
endif()
