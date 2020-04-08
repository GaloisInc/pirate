cmake_minimum_required(VERSION 3.5)

project(pnt)

if (GAPS_DISABLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGAPS_DISABLE")
endif(GAPS_DISABLE)

set(COMMON_SRC
    channel.h channel_fd.h channel_fd.cpp
    print.h print.cpp
    timer.h timer.cpp
    pnt_data.h
    sensors.h
    ownship.h ownship.cpp
    target.h target.cpp
    pnt_example.cpp
    resource_loader.cpp resource.h resource_loader.h wrapped_array.h
    empty_resources.cpp
)

if (GAPS_DISABLE)
    add_executable(pnt_green ${COMMON_SRC} green.cpp)
    add_executable(pnt_orange ${COMMON_SRC} orange.cpp)
else()
    add_executable(pnt_green ${COMMON_SRC})
    add_executable(pnt_orange ${COMMON_SRC})
    set_target_properties(pnt_green  PROPERTIES LINK_FLAGS "${LINK_FLAGS} -Wl,-enclave,green  -fuse-ld=lld")
    set_target_properties(pnt_orange PROPERTIES LINK_FLAGS "${LINK_FLAGS} -Wl,-enclave,orange -fuse-ld=lld")
endif(GAPS_DISABLE)

target_link_libraries(pnt_green ${PIRATE_APP_LIBS} pthread)
set_target_properties(pnt_green PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_link_libraries(pnt_orange ${PIRATE_APP_LIBS} pthread)
set_target_properties(pnt_orange PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

install(TARGETS pnt_green pnt_orange)
install(FILES run.sh DESTINATION bin)
configure_file(run.sh ${PROJECT_BINARY_DIR} COPYONLY)
install(FILES run_async.sh DESTINATION bin)
configure_file(run_async.sh ${PROJECT_BINARY_DIR} COPYONLY)
