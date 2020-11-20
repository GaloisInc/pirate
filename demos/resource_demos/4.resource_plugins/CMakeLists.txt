cmake_minimum_required(VERSION 3.5)
project(resource_plugins)

set(LIBRARY_OUTPUT_PATH plugins CACHE PATH "Build directory" FORCE)

add_library(foo_plugin SHARED foo.c)
set_target_properties(foo_plugin PROPERTIES
    OUTPUT_NAME foo
    PREFIX ""
    CLEAN_DIRECT_OUTPUT 1
)
target_compile_options(foo_plugin PRIVATE SHELL:-Wall SHELL:-Werror)

add_executable(plugin_app plugin_app.c)
target_link_libraries(plugin_app pal)
target_compile_options(plugin_app PRIVATE SHELL:-Wall SHELL:-Werror)
target_link_options(plugin_app PRIVATE
    SHELL:-fuse-ld=lld LINKER:-enclave,plugin_app)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cfg.yaml
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
