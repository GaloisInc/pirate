cmake_minimum_required(VERSION 3.13)
project(auto_res)

add_executable(auto_res_app auto_res_app.c)
target_link_libraries(auto_res_app pal ${PIRATE_APP_LIBS})
target_link_options(auto_res_app PRIVATE
    SHELL:-fuse-ld=lld LINKER:-enclave,my_app)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cfg.yaml
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
