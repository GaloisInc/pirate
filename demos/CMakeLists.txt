cmake_minimum_required(VERSION 3.5)

project(pirate_demos)

# Common options
option(GAPS_DISABLE "Disable compilation with GAPS annotations" OFF)
option(GAPS_DEMOS "Enable compilation of all GAPS demo applications" ON)
option(CHANNEL_DEMO "Enable compilation of GAPS channel application" OFF)
option(PIRATE_SHMEM_FEATURE "support shared memory channels" OFF)
option(PNT_DEMO "Enable compilation of pnt application" OFF)
option(CAMERA_DEMO "Enable compilation of the GAPS camera demo" OFF)
option(PIRATE_LAUNCHER "Enable the PIRATE launcher" ON)
option(PIRATE_GETOPT "Enable PIRATE getopt library" ON)
option(BUILD_ALL "Enable PIRATE_SHMEM_FEATURE, PIRATE_UNIT_TEST, GAPS_DEMOS, and GAPS_BENCH" OFF)

if(BUILD_ALL)
    SET(PIRATE_SHMEM_FEATURE ON BOOL)
    SET(PIRATE_UNIT_TEST ON BOOL)
    SET(GAPS_DEMOS ON BOOL)
    SET(GAPS_BENCH ON BOOL)
endif(BUILD_ALL)

if (NOT GAPS_DISABLE)
  # Use lld for demos if a linker is not already set.
  if (CMAKE_EXE_LINKER_FLAGS)
    string(FIND ${CMAKE_EXE_LINKER_FLAGS} -fuse-ld USE_LD_IDX)
  else()
    set(USE_LD_IDX -1)
  endif()

  if (USE_LD_IDX EQUAL -1)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
  endif()
endif()

# Common build options
set(PIRATE_C_FLAGS -Werror -Wall -Wextra -Wpedantic -fPIC)

set(PIRATE_STATIC_LIB libpirate_static)
set(PIRATE_SHARED_LIB libpirate_shared)
set(PIRATE_CXX_STATIC_LIB libpirate++_static)
set(PIRATE_CXX_SHARED_LIB libpirate++_shared)
set(PIRATE_APP_LIBS pirate)
set(PIRATE_APP_CXX_LIBS pirate++)

if (PIRATE_SHMEM_FEATURE)
    set(PIRATE_APP_LIBS ${PIRATE_APP_LIBS} pthread rt)
    set(PIRATE_APP_CXX_LIBS ${PIRATE_APP_CXX_LIBS} pthread rt)
endif(PIRATE_SHMEM_FEATURE)

if (GAPS_DEMOS)
    add_subdirectory(simple_demo)
    add_subdirectory(time_demo)
    add_subdirectory(cxx_demo)
    add_subdirectory(word_filter/unpartitioned)
endif(GAPS_DEMOS)

if (GAPS_DEMOS AND NOT GAPS_DISABLE)
    add_subdirectory(enclave_demo)
    add_subdirectory(word_filter/enclaves)
    if (PIRATE_LAUNCHER)
        add_subdirectory(word_filter/pal)
        add_subdirectory(word_filter/serviceclass)
    endif(PIRATE_LAUNCHER)
endif(GAPS_DEMOS AND NOT GAPS_DISABLE)

if (CHANNEL_DEMO OR GAPS_DEMOS)
    add_subdirectory(channel_demo)
endif(CHANNEL_DEMO OR GAPS_DEMOS)

if ((RESOURCE_DEMOS OR GAPS_DEMOS) AND NOT GAPS_DISABLE)
    add_subdirectory(resource_demos/1.manual_resources_with_pal)
    add_subdirectory(resource_demos/2.auto_resources_with_pal)
    if (PIRATE_LAUNCHER)
        add_subdirectory(resource_demos/3.channels_with_pal)
    endif(PIRATE_LAUNCHER)
endif((RESOURCE_DEMOS OR GAPS_DEMOS) AND NOT GAPS_DISABLE)

if (CAMERA_DEMO OR GAPS_DEMOS)
    add_subdirectory(camera_demo)
endif(CAMERA_DEMO OR GAPS_DEMOS)
