cmake_minimum_required(VERSION 3.14)

project(pnt)

if (GAPS_ENABLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGAPS_ENABLE")
endif(GAPS_ENABLE)

set(COMMON_SRC
    channel.h channel_fd.h channel_fd.cpp
    print.h print.cpp
    timer.h timer.cpp
    pnt_data.h
    sensors.h
    ownship.h ownship.cpp
    target.h target.cpp
    pnt_example.cpp
)

if (GAPS_ENABLE)
    add_executable(pnt_green ${COMMON_SRC})
    add_executable(pnt_orange ${COMMON_SRC})
    target_link_options(pnt_green PRIVATE "LINKER:-enclave,green")
    target_link_options(pnt_orange PRIVATE "LINKER:-enclave,orange")
else()
    add_executable(pnt_green ${COMMON_SRC} green.cpp)
    add_executable(pnt_orange ${COMMON_SRC} orange.cpp)
endif(GAPS_ENABLE)

target_link_libraries(pnt_green ${PIRATE_APP_LIBS})
set_target_properties(pnt_green PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_link_libraries(pnt_orange ${PIRATE_APP_LIBS})
set_target_properties(pnt_orange PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

install(TARGETS pnt_green pnt_orange)
install(FILES run.sh DESTINATION bin)
configure_file(run.sh ${PROJECT_BINARY_DIR} COPYONLY)
