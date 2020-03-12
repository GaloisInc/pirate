cmake_minimum_required(VERSION 3.14)

project(pnt)

add_executable(pnt
    observer.h
    pnt_data.h
    sensors.h
    ownship.h ownship.cpp
    target.h target.cpp
    pnt_example.cpp
)

set_target_properties(pnt PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

