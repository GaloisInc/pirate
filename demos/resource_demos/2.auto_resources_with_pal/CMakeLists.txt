cmake_minimum_required(VERSION 3.5)
project(auto_res)

add_executable(auto_res_app auto_res_app.c)
target_link_libraries(auto_res_app libpal_shared)
target_compile_options(auto_res_app PRIVATE SHELL:-Wall SHELL:-Werror)
target_link_options(auto_res_app PRIVATE
    SHELL:-fuse-ld=lld LINKER:-enclave,my_app)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cfg.yaml
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
