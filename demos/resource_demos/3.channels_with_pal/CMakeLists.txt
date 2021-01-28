cmake_minimum_required(VERSION 3.13)
project(channels_with_pal)

add_executable(channel_app1 channel_app.c)
target_link_libraries(channel_app1 pal ${PIRATE_APP_LIBS})
target_link_options(channel_app1 PRIVATE
    SHELL:-fuse-ld=lld LINKER:-enclave,channel_app1)

add_executable(channel_app2 channel_app.c)
target_link_libraries(channel_app2 pal ${PIRATE_APP_LIBS})
target_link_options(channel_app2 PRIVATE
    SHELL:-fuse-ld=lld LINKER:-enclave,channel_app2)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/unix_socket.yaml
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
