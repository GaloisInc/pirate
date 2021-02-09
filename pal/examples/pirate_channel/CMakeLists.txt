add_executable(pal_client client.c)
target_compile_options(pal_client PRIVATE -ffunction-sections -fdata-sections)
target_link_libraries(pal_client ${PIRATE_APP_LIBS} pthread libpal_shared)
set_target_properties(pal_client PROPERTIES LINK_FLAGS "${LINK_FLAGS} -Wl,-enclave,client -fuse-ld=lld")

add_executable(pal_server server.c)
target_compile_options(pal_server PRIVATE -ffunction-sections -fdata-sections)
target_link_libraries(pal_server ${PIRATE_APP_LIBS} pthread libpal_shared)
set_target_properties(pal_server PROPERTIES LINK_FLAGS "${LINK_FLAGS} -Wl,-enclave,server -fuse-ld=lld")


configure_file(pal.yaml pal.yaml COPYONLY)