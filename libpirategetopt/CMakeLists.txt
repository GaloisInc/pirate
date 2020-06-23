cmake_minimum_required(VERSION 3.10)

project(pirategetopt)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(pirategetopt_static STATIC
  resource.hpp
  wrapped_array.hpp
  resource_loader.cpp
  include/resource_loader.hpp
)

target_include_directories(pirategetopt_static PUBLIC include)
set_target_properties(pirategetopt_static PROPERTIES OUTPUT_NAME pirategetopt CLEAN_DIRECT_OUTPUT 1)

install(FILES include/resource_loader.hpp DESTINATION include)
install(TARGETS pirategetopt_static DESTINATION lib)
