cmake_minimum_required(VERSION 3.5)
include(CheckIncludeFile)

# This must be done before any language is set (ie before any project() or
# enable_language() command).
# https://gitlab.kitware.com/cmake/community/-/wikis/FAQ#method-3-avoid-use-set
if (NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER "clang")
endif(NOT CMAKE_C_COMPILER)

project(gaps)

# Common options
option(GAPS_DISABLE "Disable compilation with GAPS annotations" OFF)
option(SINGLE_BINARY "Create a single binary across enclaves" OFF)
option(PIRATE_SHMEM_FEATURE "support shared memory channels" OFF)
option(PIRATE_UNIT_TEST "Enable compilation of PIRATE unit tests" OFF)
option(GAPS_DEMOS "Enable compilation of all GAPS demo applications" OFF)
option(CHANNEL_DEMO "Enable compilation of GAPS channel application" OFF)
option(PNT_DEMO "Enable compilation of pnt application" OFF)
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
set(PIRATE_APP_LIBS ${PIRATE_STATIC_LIB})

if (PIRATE_SHMEM_FEATURE)
    set(PIRATE_LIB_LIBS ${PIRATE_LIB_LIBS} pthread rt)
    SET(PIRATE_C_FLAGS ${PIRATE_C_FLAGS} -mavx2)
    set(PIRATE_APP_LIBS ${PIRATE_APP_LIBS} pthread rt)
endif(PIRATE_SHMEM_FEATURE)

CHECK_INCLUDE_FILE(stdatomic.h HAVE_STD_ATOMIC)
if (HAVE_STD_ATOMIC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_STD_ATOMIC")
endif(HAVE_STD_ATOMIC)

add_subdirectory(libpirate)
include_directories(BEFORE libpirate)
link_directories(BEFORE ${CMAKE_BINARY_DIR}/libpirate)
add_subdirectory(demos)
