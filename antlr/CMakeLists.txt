cmake_minimum_required(VERSION 3.5)
project(idl-codegen VERSION 0.1)

set(CMAKE_CXX_STANDARD 11)

if(NOT DEFINED ANTLR4_MODULE_PATH)
    message(FATAL_ERROR "ANTLR4_MODULE_PATH must be defined")
endif(NOT DEFINED ANTLR4_MODULE_PATH)

if(NOT DEFINED ANTLR4_JAR_LOCATION)
    message(FATAL_ERROR "ANTLR4_JAR_LOCATION must be defined")
endif(NOT DEFINED ANTLR4_JAR_LOCATION)

set(CMAKE_MODULE_PATH ${ANTLR4_MODULE_PATH})
set(IDL_CXX_FLAGS -Werror -Wall -Wextra -Wpedantic -Wno-attributes -Wno-unused-parameter)

include(antlr4-runtime-config)
include_directories(${ANTLR4_INCLUDE_DIR})
link_directories(${ANTLR4_LIB_DIR})

include(antlr4-generator-config)
antlr4_generate(IDL ${CMAKE_CURRENT_SOURCE_DIR}/IDL.g4 BOTH -listener -visitor)
include_directories(${ANTLR4_INCLUDE_DIR_IDL})

SET(CDR_SOURCES
    "CDRBuildTypes.cpp"
    "CDRGenerator.cpp"
    "CDRTypes.cpp"
)

add_executable(CDRGenerator ${CDR_SOURCES} ${ANTLR4_SRC_FILES_IDL})
target_compile_options(CDRGenerator PRIVATE ${IDL_CXX_FLAGS})
target_link_libraries(CDRGenerator antlr4-runtime)

