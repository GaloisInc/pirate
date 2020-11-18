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
option(PIRATE_LLVM_UBUNTU "Disable ipv6 unit tests" OFF)
option(GAPS_DEMOS "Enable compilation of all GAPS demo applications" OFF)
option(CHANNEL_DEMO "Enable compilation of GAPS channel application" OFF)
option(PNT_DEMO "Enable compilation of pnt application" OFF)
option(RESOURCE_DEMOS "Enable compilation of resource and pal demos" OFF)
option(GAPS_BENCH "Enable compilation of GAPS benchmark applications" OFF)
option(PIRATE_LAUNCHER "Enable the PIRATE launcher" ON)
option(PIRATE_GETOPT "Enable PIRATE getopt library" ON)
option(OBJECT_MODEL "Enable object model exploration" OFF)
option(BUILD_ALL "Enable PIRATE_SHMEM_FEATURE, PIRATE_UNIT_TEST, GAPS_DEMOS, and GAPS_BENCH" OFF)

if(BUILD_ALL)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86")
        SET(PIRATE_SHMEM_FEATURE ON BOOL)
    endif()
    SET(PIRATE_UNIT_TEST ON BOOL)
    SET(GAPS_DEMOS ON BOOL)
    SET(CHANNEL_DEMO ON BOOL)
    SET(PNT_DEMO ON BOOL)
    SET(RESOURCE_DEMOS ON BOOL)
    SET(GAPS_BENCH ON BOOL)
    SET(OBJECT_MODEL ON BOOL)
endif(BUILD_ALL)

# Common build options
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(PIRATE_C_FLAGS /W3 /MT)
else()
    set(PIRATE_C_FLAGS -Werror -Wall -Wextra -Wpedantic -fPIC)
endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

set(PIRATE_STATIC_LIB libpirate_static)
set(PIRATE_SHARED_LIB libpirate_shared)
set(PIRATE_APP_LIBS ${PIRATE_STATIC_LIB})

if (PIRATE_SHMEM_FEATURE)
    set(PIRATE_LIB_LIBS ${PIRATE_LIB_LIBS} pthread rt)
    if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "x86")
        message(FATAL_ERROR "SHMEM feature is supported for x86 only")
    endif()
    set(PIRATE_C_FLAGS ${PIRATE_C_FLAGS} -mavx2)
    set(PIRATE_APP_LIBS ${PIRATE_APP_LIBS} pthread rt)
    set(PIRATE_APP_CXX_LIBS ${PIRATE_APP_CXX_LIBS} pthread rt)
endif(PIRATE_SHMEM_FEATURE)

CHECK_INCLUDE_FILE(stdatomic.h HAVE_STD_ATOMIC)
if (HAVE_STD_ATOMIC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_STD_ATOMIC")
endif(HAVE_STD_ATOMIC)

# Add libpirate to build
add_subdirectory(libpirate)

# Add libpirate include and link directories for other directories.
include_directories(BEFORE libpirate)
link_directories(BEFORE ${CMAKE_BINARY_DIR}/libpirate)

# Build application launcher
if (PIRATE_LAUNCHER)
    add_subdirectory(pal)
    include_directories(BEFORE pal/include)
    link_directories(BEFORE ${CMAKE_BINARY_DIR}/pal)
endif(PIRATE_LAUNCHER)

# Build getopt library for working with resources.
if (PIRATE_GETOPT)
  add_subdirectory(libpirategetopt)
  include_directories(BEFORE libpirategetopt/include)
  link_directories(BEFORE ${CMAKE_BINARY_DIR}/libpirategetopt)
endif(PIRATE_GETOPT)

# Build demos
if (GAPS_DEMOS OR CHANNEL_DEMO OR PNT_DEMO OR RESOURCE_DEMOS OR CAMERA_DEMO)
    add_subdirectory(demos)
endif(GAPS_DEMOS OR CHANNEL_DEMO OR PNT_DEMO OR RESOURCE_DEMOS OR CAMERA_DEMO)
