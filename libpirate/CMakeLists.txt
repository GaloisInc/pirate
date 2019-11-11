cmake_minimum_required(VERSION 3.5)
project(libpirate)

set(CMAKE_BUILD_TYPE RelwithDebInfo)
SET(PIRATE_SHMEM_FEATURE 0)

if(PIRATE_SHMEM_FEATURE)
add_definitions(-DPIRATE_SHMEM_FEATURE=1)
SET(PIRATE_SOURCES "primitives.c" "shmem_interface.c" "shmem.c")
else()
SET(PIRATE_SOURCES "primitives.c" "shmem_interface.c")
endif(PIRATE_SHMEM_FEATURE)

include_directories(. ../include)

add_executable(primitives_test ${PIRATE_SOURCES} test/primitives_test.c test/shmem_test.c)

add_executable(primitives_bench ${PIRATE_SOURCES} bench/primitives_bench.c)

add_library(pirateprims-static STATIC ${PIRATE_SOURCES})
set_target_properties(pirateprims-static PROPERTIES OUTPUT_NAME pirate CLEAN_DIRECT_OUTPUT 1)

add_library(pirateprims-shared SHARED ${PIRATE_SOURCES})
set_target_properties(pirateprims-shared PROPERTIES OUTPUT_NAME pirate CLEAN_DIRECT_OUTPUT 1)

target_compile_options(primitives_test PRIVATE -Werror -Wall -Wextra -Wpedantic)
target_compile_options(primitives_bench PRIVATE -Werror -Wall -Wextra -Wpedantic)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(PIRATE_SHMEM_FEATURE)
target_link_libraries(primitives_test rt Threads::Threads)
target_link_libraries(primitives_bench rt Threads::Threads)
else()
target_link_libraries(primitives_test Threads::Threads)
target_link_libraries(primitives_bench Threads::Threads)
endif(PIRATE_SHMEM_FEATURE)

enable_testing()

add_test(NAME primitives_test COMMAND primitives_test -v)

add_custom_target(testing COMMAND ${CMAKE_CTEST_COMMAND}
    --force-new-ctest-process
    --verbose
    --output-on-failure
)
