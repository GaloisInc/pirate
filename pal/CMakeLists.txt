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

add_library(libpal_shared SHARED lib/pal/pal.c lib/pal/envelope.c)
set_target_properties(libpal_shared PROPERTIES OUTPUT_NAME libpal CLEAN_DIRECT_OUTPUT 1)
target_include_directories(libpal_shared PUBLIC include)
target_link_libraries(libpal_shared ${PIRATE_APP_LIBS})
set_property(TARGET libpal_shared PROPERTY PREFIX "")
target_compile_options(libpal_shared PRIVATE -Wall -Werror)

install(TARGETS libpal_shared DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/pal DESTINATION include)

###
# pirate_channel resource plugin
###

add_library(pirate_channel_plugin SHARED lib/plugins/pirate_channel.c)
set_target_properties(pirate_channel_plugin
    PROPERTIES OUTPUT_NAME pirate_channel LIBRARY_OUTPUT_DIRECTORY plugins CLEAN_DIRECT_OUTPUT 1)
target_include_directories(pirate_channel_plugin PRIVATE include)
target_link_libraries(pirate_channel_plugin ${PIRATE_APP_LIBS})
set_property(TARGET pirate_channel_plugin PROPERTY PREFIX "")
target_compile_options(pirate_channel_plugin PRIVATE -Wall -Werror -g)

install(TARGETS pirate_channel_plugin DESTINATION lib/pirate/pal/plugins)

###
# pal executable
###

add_executable(exepal
    src/main.c
    src/handle_apps.c src/handle_apps.h
    src/launch.c src/launch.h
    src/log.c src/log.h
    src/yaml.c src/yaml.h
    lib/pal/envelope.c include/pal/envelope.h
)
set_target_properties(exepal PROPERTIES OUTPUT_NAME pal)
target_include_directories(exepal PRIVATE include)
target_link_libraries(exepal yaml dl)
target_compile_options(exepal PRIVATE -Wall -Werror -g)
target_link_options(exepal PRIVATE
    SHELL:-Wl,-export-dynamic # Allows plugins to access symbols defined in pal
)

install(TARGETS exepal DESTINATION bin)
install(FILES include/pal/pal.h DESTINATION include/pal)

if (NOT GAPS_DISABLE)
  add_subdirectory(examples/pirate_channel)
endif(NOT GAPS_DISABLE)
