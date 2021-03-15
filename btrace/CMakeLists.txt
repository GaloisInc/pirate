project(btrace)
cmake_minimum_required(VERSION 3.0)

add_executable(btrace src/tracer.cpp src/btrace.cpp)
target_include_directories(btrace PRIVATE deps/rapidjson-1.1.0/include)