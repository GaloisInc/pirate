cmake_minimum_required(VERSION 3.5)

set(CMAKE_BUILD_TYPE Debug)

project(pnt.yield)

if (GAPS_DISABLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGAPS_DISABLE")
endif(GAPS_DISABLE)

set(COMMON_SRC
    channel.h
    print.h print.cpp
    pnt_data.h
    sensors.h
    ownship.h ownship.cpp
    target.h target.cpp
    main.cpp
)

if (GAPS_DISABLE)
    add_executable(pnt_yield_green ${COMMON_SRC} green.cpp)
    add_executable(pnt_yield_orange ${COMMON_SRC} orange.cpp)
else()
    add_executable(pnt_yield_green  ${COMMON_SRC})
    add_executable(pnt_yield_orange ${COMMON_SRC})
    set_target_properties(pnt_yield_green  PROPERTIES LINK_FLAGS "${LINK_FLAGS} -Wl,-enclave,green  -fuse-ld=lld")
    set_target_properties(pnt_yield_orange PROPERTIES LINK_FLAGS "${LINK_FLAGS} -Wl,-enclave,orange -fuse-ld=lld")
endif(GAPS_DISABLE)

target_link_libraries(pnt_yield_green ${PIRATE_APP_CXX_LIBS})
set_target_properties(pnt_yield_green PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_link_libraries(pnt_yield_orange ${PIRATE_APP_CXX_LIBS})
set_target_properties(pnt_yield_orange PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

install(TARGETS pnt_yield_green pnt_yield_orange DESTINATION bin)
install(FILES run.sh DESTINATION bin)
configure_file(run.sh ${PROJECT_BINARY_DIR} COPYONLY)
