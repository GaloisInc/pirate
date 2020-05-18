cmake_minimum_required(VERSION 3.5)
project(resource_demos/2.auto_resources_with_pal)

add_executable(app app.c)
target_link_libraries(app pal)
target_link_options(app PRIVATE SHELL:-Wall SHELL:-Werror SHELL:-g
    SHELL:-fuse-ld=lld LINKER:-enclave,my_app)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cfg.yaml
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
