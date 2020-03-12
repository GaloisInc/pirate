cmake_minimum_required(VERSION 3.14)

project(pnt)

add_executable(pnt_green
    channel.h
    pnt_data.h
    sensors.h
    ownship.h ownship.cpp
    target.h target.cpp
    pnt_example.cpp
)
target_link_libraries(pnt_green "pthread" "pirate")
target_link_options(pnt_green PRIVATE "LINKER:-enclave,green")
set_target_properties(pnt_green PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

add_executable(pnt_orange
    channel.h
    pnt_data.h
    sensors.h
    ownship.h ownship.cpp
    target.h target.cpp
    pnt_example.cpp
)
target_link_libraries(pnt_orange "pthread" "pirate")
target_link_options(pnt_orange PRIVATE "LINKER:-enclave,orange")
set_target_properties(pnt_orange PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

install(TARGETS pnt_green pnt_orange)
