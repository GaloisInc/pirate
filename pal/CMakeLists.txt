cmake_minimum_required(VERSION 3.5)

if (NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER "clang")
endif(NOT CMAKE_C_COMPILER)

project(pal)

###
# Dependencies
###

#find_library(YAML_LIB yaml-0.1)
#if(NOT YAML_LIB)
#    message(FATAL_ERROR "libyaml not found")
#endif(NOT YAML_LIB)

###
# libpal shared library
###

add_library(libpal_shared SHARED
    lib/pal/pal.c include/pal/pal.h
    lib/pal/envelope.c include/pal/envelope.h
)
set_target_properties(libpal_shared
    PROPERTIES OUTPUT_NAME libpal CLEAN_DIRECT_OUTPUT 1)
target_include_directories(libpal_shared PUBLIC include)
target_link_libraries(libpal_shared ${PIRATE_APP_LIBS})
set_property(TARGET libpal_shared PROPERTY PREFIX "")
target_compile_options(libpal_shared PRIVATE -Wall -Werror)

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
    include
    ${CYAML_INSTALL_DIR}/include
)

target_link_libraries(exepal libcyaml ${PIRATE_APP_LIBS} pthread rt)
target_compile_options(exepal PRIVATE -Wall -Werror -export-dynamic)

install(TARGETS exepal DESTINATION bin)
install(FILES include/pal/pal.h DESTINATION include/pal)