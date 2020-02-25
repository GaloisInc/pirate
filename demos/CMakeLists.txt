cmake_minimum_required(VERSION 3.13)
project(pirate_demos)
option(GAPS_ENABLE "Enable compilation with GAPS annotation" OFF)
option(SINGLE_BINARY "Create a single binary across enclaves" OFF)

add_subdirectory(simple_demo)
add_subdirectory(time_demo)
add_subdirectory(channel_demo)
add_subdirectory(cxx_demo)

if (GAPS_ENABLE)
    add_subdirectory(enclave_demo)
endif(GAPS_ENABLE)
