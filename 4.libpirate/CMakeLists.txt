cmake_minimum_required(VERSION 3.14)

project(pnt)

add_executable(green
    channel.h
    pnt_data.h
    sensors.h
    ownship.h ownship.cpp
    target.h target.cpp
    pnt_example.cpp
)
target_link_libraries(green "pthread" "pirate")
target_link_options(green PRIVATE "LINKER:-enclave,green")
set_target_properties(green PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

add_executable(yellow
    channel.h
    pnt_data.h
    sensors.h
    ownship.h ownship.cpp
    target.h target.cpp
    pnt_example.cpp
)
target_link_libraries(yellow "pthread" "pirate")
target_link_options(yellow PRIVATE "LINKER:-enclave,yellow")
set_target_properties(yellow PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
