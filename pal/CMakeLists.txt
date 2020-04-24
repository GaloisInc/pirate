cmake_minimum_required(VERSION 3.15)
project(pal)

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
)

add_library(libcyaml SHARED IMPORTED)
ExternalProject_Get_Property(project_libcyaml install_dir)
set(CYAML_INSTALL_DIR ${install_dir})
add_dependencies(libcyaml project_libcyaml)
set_property(TARGET libcyaml PROPERTY IMPORTED_LOCATION
    ${CYAML_INSTALL_DIR}/lib/libcyaml.so)

###
# libpal shared library
###

add_library(libpal SHARED
    lib/pal/pal.c include/pal/pal.h
    lib/pal/envelope.c include/pal/envelope.h
)
target_include_directories(libpal PRIVATE include)
set_property(TARGET libpal PROPERTY PREFIX "")

###
# pal executable
###

add_executable(pal
    src/main.c
    src/handle_apps.c src/handle_apps.h
    src/handlers.c src/handlers.h
    src/launch.c src/launch.h
    src/log.c src/log.h
    src/yaml.c src/yaml.h
    lib/pal/envelope.c include/pal/envelope.h
)
target_include_directories(pal PRIVATE
    include
    ${CYAML_INSTALL_DIR}/include
)
target_link_libraries(pal libcyaml)
