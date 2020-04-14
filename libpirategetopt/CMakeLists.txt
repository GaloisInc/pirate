cmake_minimum_required(VERSION 3.10)

project(pirategetopt)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(pirategetopt STATIC
  resource.hpp
  wrapped_array.hpp
  resource_loader.cpp
  include/resource_loader.hpp
)

target_include_directories(pirategetopt PUBLIC include)
