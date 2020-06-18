cmake_minimum_required(VERSION 3.5)
project(idl-codegen VERSION 0.1)

set(CMAKE_CXX_STANDARD 11)

if(NOT DEFINED ANTLR4_MODULE_PATH)
    message(FATAL_ERROR "ANTLR4_MODULE_PATH must be defined (eg. /usr/local/lib/cmake/antlr4)")
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

add_executable(CDRGenerator CDRGeneratorMain.cpp ${CDR_SOURCES} ${ANTLR4_SRC_FILES_IDL})
target_compile_options(CDRGenerator PRIVATE ${IDL_CXX_FLAGS})
target_link_libraries(CDRGenerator antlr4-runtime)

if(PIRATE_UNIT_TEST)
    find_package(GTest REQUIRED)
    include_directories(. ${GTEST_INCLUDE_DIR})

    file(GLOB IDL_TEST_SRC test/*.cpp)
    add_executable(idl_test ${IDL_TEST_SRC} ${CDR_SOURCES} ${ANTLR4_SRC_FILES_IDL})
    target_compile_options(idl_test PRIVATE ${IDL_CXX_FLAGS})
    target_link_libraries(idl_test ${GTEST_MAIN_LIBRARY} ${GTEST_LIBRARIES} antlr4-runtime pthread)

    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test/input DESTINATION .)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test/output DESTINATION .)

    add_custom_target(valgrind COMMAND valgrind
        --leak-check=full
        --error-exitcode=1
        ./idl_test
    )
endif(PIRATE_UNIT_TEST)
