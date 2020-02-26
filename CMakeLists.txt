cmake_minimum_required(VERSION 3.13)
project(gaps)

# Common options
option(GAPS_ENABLE "Enable compilation with GAPS annotation" OFF)
option(SINGLE_BINARY "Create a single binary across enclaves" OFF)
option(GAPS_DEMOS "Enable compilation of GAPS demo applications" OFF)

add_subdirectory(libpirate)

if (GAPS_DEMOS)
    include_directories(${CMAKE_SOURCE_DIR}/libpirate)
    link_directories(${CMAKE_BINARY_DIR}/libpirate)

    add_subdirectory(demos/simple_demo)
    add_subdirectory(demos/time_demo)
    add_subdirectory(demos/channel_demo)
    add_subdirectory(demos/cxx_demo)

    if (GAPS_ENABLE)
        add_subdirectory(demos/enclave_demo)
    endif(GAPS_ENABLE)

endif(GAPS_DEMOS)
