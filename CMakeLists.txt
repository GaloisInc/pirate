cmake_minimum_required(VERSION 3.13)
include(CheckIncludeFile)

# This must be done before any language is set (ie before any project() or
# enable_language() command).
# https://gitlab.kitware.com/cmake/community/-/wikis/FAQ#method-3-avoid-use-set
if (NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER "clang")
endif(NOT CMAKE_C_COMPILER)

project(gaps)

# Common options
option(GAPS_ENABLE "Enable compilation with GAPS annotation" OFF)
option(SINGLE_BINARY "Create a single binary across enclaves" OFF)
option(PIRATE_SHMEM_FEATURE "support shared memory channels" OFF)
option(PIRATE_UNIT_TEST "Enable compilation of PIRATE unit tests" OFF)
option(CHANNEL_DEMO "Enable compilation of GAPS channel application" OFF)
option(GAPS_DEMOS "Enable compilation of all GAPS demo applications" OFF)
option(GAPS_BENCH "Enable compilation of GAPS benchmark applications" OFF)
option(BUILD_ALL "Enable PIRATE_SHMEM_FEATURE, PIRATE_UNIT_TEST, GAPS_DEMOS, and GAPS_BENCH" OFF)

if(BUILD_ALL)
    SET(PIRATE_SHMEM_FEATURE ON BOOL)
    SET(PIRATE_UNIT_TEST ON BOOL)
    SET(GAPS_DEMOS ON BOOL)
    SET(GAPS_BENCH ON BOOL)
endif(BUILD_ALL)

# Common build options
set(PIRATE_C_FLAGS -Werror -Wall -Wextra -Wpedantic -fPIC)

set(PIRATE_STATIC_LIB libpirate_static)
set(PIRATE_SHARED_LIB libpirate_shared)
set(PIRATE_APP_LIBS ${PIRATE_STATIC_LIB} pthread)

if (PIRATE_SHMEM_FEATURE)
    set(PIRATE_LIB_LIBS ${PIRATE_LIB_LIBS} pthread rt)
    SET(PIRATE_C_FLAGS ${PIRATE_C_FLAGS} -mavx2)
    set(PIRATE_APP_LIBS ${PIRATE_APP_LIBS} rt)
endif(PIRATE_SHMEM_FEATURE)

CHECK_INCLUDE_FILE(stdatomic.h HAVE_STD_ATOMIC)
if (HAVE_STD_ATOMIC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_STD_ATOMIC")
endif(HAVE_STD_ATOMIC)

add_subdirectory(libpirate)

if (CHANNEL_DEMO)
    include_directories(${CMAKE_SOURCE_DIR}/libpirate)
    link_directories(${CMAKE_BINARY_DIR}/libpirate)
    add_subdirectory(demos/channel_demo)
endif(CHANNEL_DEMO)

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
