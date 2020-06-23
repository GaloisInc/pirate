cmake_minimum_required(VERSION 3.5)

set(CMAKE_BUILD_TYPE Debug)

project(pnt)

set(COMMON_SRC
    channel.h channel_fd.h channel_fd.cpp
    print.h print.cpp
    timer.h timer.cpp
    pnt_data.h
    sensors.h
    ownship.h ownship.cpp
    target.h target.cpp
    main.cpp
)

add_executable(        pnt_getopt_green ${COMMON_SRC})
target_compile_options(pnt_getopt_green PRIVATE -ffunction-sections -fdata-sections)
target_link_libraries( pnt_getopt_green pirate pthread pirategetopt)
set_target_properties( pnt_getopt_green PROPERTIES
    LINK_FLAGS "${LINK_FLAGS} -Wl,-enclave,green  -fuse-ld=lld"
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

add_executable(        pnt_getopt_orange ${COMMON_SRC})
target_compile_options(pnt_getopt_orange PRIVATE -ffunction-sections -fdata-sections)
target_link_libraries( pnt_getopt_orange pirate pthread pirategetopt)
set_target_properties( pnt_getopt_orange PROPERTIES
    LINK_FLAGS "${LINK_FLAGS} -Wl,-enclave,orange -fuse-ld=lld"
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

install(TARGETS pnt_getopt_green pnt_getopt_orange)
configure_file(scripts/pnt_pal_run_local.sh ${PROJECT_BINARY_DIR} COPYONLY)