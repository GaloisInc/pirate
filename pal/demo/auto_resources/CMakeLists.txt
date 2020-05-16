cmake_minimum_required(VERSION 3.5)
project(app)

add_executable(app app.c ../../include/pal/pal.h)
target_include_directories(app PRIVATE ../../include)
target_link_libraries(app pal)
target_link_options(app PRIVATE SHELL:-Wall SHELL:-Werror SHELL:-g
    SHELL:-fuse-ld=lld LINKER:-enclave,my_app)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cfg.yaml
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
