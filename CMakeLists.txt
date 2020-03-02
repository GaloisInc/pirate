cmake_minimum_required(VERSION 3.13)
project(gaps)

# Common options
option(GAPS_ENABLE "Enable compilation with GAPS annotation" OFF)
option(SINGLE_BINARY "Create a single binary across enclaves" OFF)
OPTION(PIRATE_SHMEM_FEATURE "support shared memory channels" OFF)

# Common build options
set(PIRATE_STATIC_LIB pirateprims-static)
set(PIRATE_LIB_LIBS)

SET(PIRATE_C_FLAGS -Werror -Wall -Wextra -Wpedantic -fPIC)

if (PIRATE_SHMEM_FEATURE)
    set(PIRATE_LIB_LIBS ${PIRATE_LIB_LIBS} pthread rt)
    SET(PIRATE_C_FLAGS ${PIRATE_C_FLAGS} -mavx2)
endif(PIRATE_SHMEM_FEATURE)

set(PIRATE_APP_LIBS ${PIRATE_STATIC_LIB} ${PIRATE_LIB_LIBS})

add_subdirectory(libpirate)
add_subdirectory(demos/simple_demo)
add_subdirectory(demos/time_demo)
add_subdirectory(demos/channel_demo)
add_subdirectory(demos/cxx_demo)

if (GAPS_ENABLE)
    add_subdirectory(demos/enclave_demo)
endif(GAPS_ENABLE)

