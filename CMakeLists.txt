cmake_minimum_required(VERSION 3.13)
project(gaps)

# Common options
option(GAPS_ENABLE "Enable compilation with GAPS annotation" OFF)
option(SINGLE_BINARY "Create a single binary across enclaves" OFF)

add_subdirectory(libpirate)
add_subdirectory(demos/simple_demo)
add_subdirectory(demos/time_demo)

if (GAPS_ENABLE)
    add_subdirectory(demos/enclave_demo)
endif(GAPS_ENABLE)
