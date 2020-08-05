cmake_minimum_required(VERSION 3.5)

project(pal)

###
# Dependencies
###

#find_library(YAML_LIB yaml-0.1)
#if(NOT YAML_LIB)
#    message(FATAL_ERROR "libyaml not found")
#endif(NOT YAML_LIB)

###
# Adding libcyaml as an external project, ala
# https://mirkokiefer.com/cmake-by-example-f95eb47d45b1
###

include(ExternalProject)
ExternalProject_Add(project_libcyaml
    GIT_REPOSITORY https://github.com/tlsa/libcyaml
    GIT_TAG v1.0.2
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/libcyaml-1.0.2
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ""
    BUILD_COMMAND make VARIANT=release
    INSTALL_COMMAND make install
    PREFIX=${CMAKE_CURRENT_BINARY_DIR}/libcyaml-1.0.2
    BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/libcyaml-1.0.2/lib/libcyaml.so
)

add_library(libcyaml_shared SHARED IMPORTED)
ExternalProject_Get_Property(project_libcyaml install_dir)
set(CYAML_INSTALL_DIR ${install_dir})
add_dependencies(libcyaml_shared project_libcyaml)
set_property(TARGET libcyaml_shared
    PROPERTY IMPORTED_LOCATION ${CYAML_INSTALL_DIR}/lib/libcyaml.so)

install(DIRECTORY ${CYAML_INSTALL_DIR}/lib/ DESTINATION lib
    USE_SOURCE_PERMISSIONS FILES_MATCHING PATTERN "libcyaml.so*")

if(PAL_UNIT_TEST)
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIR})

    add_executable(pal_unit_test
        test/unit/main.cpp test/unit/yaml.cpp test/unit/handlers.cpp
        test/unit/pal.cpp test/unit/envelope.cpp
        src/yaml.c src/log.c src/handlers.c
        lib/pal/pal.c lib/pal/envelope.c)
    target_link_libraries(pal_unit_test
        ${GTEST_LIBRARY} libcyaml_shared pirate)
    target_include_directories(pal_unit_test PRIVATE
        ${CYAML_INSTALL_DIR}/include include src)
    target_compile_options(pal_unit_test PRIVATE
        SHELL:-Wall SHELL:-Werror)
    target_link_options(pal_unit_test PRIVATE
        SHELL:-fuse-ld=lld LINKER:-enclave=test)
    add_test(NAME pal_unit COMMAND pal_unit_test)
endif(PAL_UNIT_TEST)

###
# libpal shared library
###

add_library(libpal_shared SHARED
    lib/pal/pal.c include/pal/pal.h
    lib/pal/envelope.c include/pal/envelope.h
)
set_target_properties(libpal_shared
    PROPERTIES OUTPUT_NAME libpal CLEAN_DIRECT_OUTPUT 1)
target_include_directories(libpal_shared PRIVATE include)
target_link_libraries(libpal_shared pirate)
set_property(TARGET libpal_shared PROPERTY PREFIX "")
target_compile_options(libpal_shared PRIVATE SHELL:-Wall SHELL:-Werror)

install(TARGETS libpal_shared DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/pal DESTINATION include)

###
# pal executable
###

add_executable(exepal
    src/main.c
    src/handle_apps.c src/handle_apps.h
    src/handlers.c src/handlers.h
    src/launch.c src/launch.h
    src/log.c src/log.h
    src/yaml.c src/yaml.h
    lib/pal/envelope.c include/pal/envelope.h
)
set_target_properties(exepal PROPERTIES OUTPUT_NAME pal)
target_include_directories(exepal PRIVATE
    include ${CYAML_INSTALL_DIR}/include
)
target_link_libraries(exepal libcyaml_shared pirate)
target_compile_options(exepal PRIVATE SHELL:-Wall SHELL:-Werror)

install(TARGETS exepal DESTINATION bin)
install(FILES include/pal/pal.h DESTINATION include/pal)
