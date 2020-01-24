cmake_minimum_required(VERSION 3.5)
project(libpirate)

set(CMAKE_C_COMPILER "clang")

set(CMAKE_BUILD_TYPE RelwithDebInfo)
SET(PIRATE_SHMEM_FEATURE 0)

SET(PIRATE_SOURCES
    "primitives.c"
    "serial.c"
    "shmem_interface.c"
    "shmem_udp_interface.c"
    "tcp_socket.c" 
    "udp_socket.c" 
    "unix_socket.c" 
    "uio.c"
)

SET(PIRATE_C_FLAGS -Werror -Wall -Wextra -Wpedantic -fPIC)

if(PIRATE_SHMEM_FEATURE)
add_definitions(-DPIRATE_SHMEM_FEATURE=1)
set(PIRATE_SOURCES ${PIRATE_SOURCES} "shmem.c" "shmem_udp.c" "checksum.c")
SET(PIRATE_C_FLAGS ${PIRATE_C_FLAGS} -mavx2)
endif(PIRATE_SHMEM_FEATURE)

include_directories(. ../include)

add_executable(primitives_test 
    ${PIRATE_SOURCES}
    test/primitives_test.c
    test/serial_test.c
    test/shmem_test.c
    test/shmem_udp_test.c
    test/uio_test.c
)

add_executable(primitives_bench_thr ${PIRATE_SOURCES} bench/primitives_bench_thr.c)
add_executable(primitives_bench_thr_vector ${PIRATE_SOURCES} bench/primitives_bench_thr_vector.c)
add_executable(primitives_bench_lat ${PIRATE_SOURCES} bench/primitives_bench_lat.c)
add_executable(primitives_bench_lat_percentiles ${PIRATE_SOURCES} bench/primitives_bench_lat_percentiles.c)
set_target_properties(primitives_bench_thr primitives_bench_thr_vector primitives_bench_lat primitives_bench_lat_percentiles PROPERTIES EXCLUDE_FROM_ALL 1)
add_custom_target(bench DEPENDS primitives_bench_thr primitives_bench_thr_vector primitives_bench_lat primitives_bench_lat_percentiles)

add_library(pirateprims-static STATIC ${PIRATE_SOURCES})
set_target_properties(pirateprims-static PROPERTIES OUTPUT_NAME pirate CLEAN_DIRECT_OUTPUT 1)

add_library(pirateprims-shared SHARED ${PIRATE_SOURCES})
set_target_properties(pirateprims-shared PROPERTIES OUTPUT_NAME pirate CLEAN_DIRECT_OUTPUT 1)

target_compile_options(pirateprims-static PRIVATE ${PIRATE_C_FLAGS})
target_compile_options(pirateprims-shared PRIVATE ${PIRATE_C_FLAGS})

target_compile_options(primitives_test PRIVATE ${PIRATE_C_FLAGS})
target_compile_options(primitives_bench_thr PRIVATE ${PIRATE_C_FLAGS})
target_compile_options(primitives_bench_thr_vector PRIVATE ${PIRATE_C_FLAGS})
target_compile_options(primitives_bench_lat PRIVATE ${PIRATE_C_FLAGS})
target_compile_options(primitives_bench_lat_percentiles PRIVATE ${PIRATE_C_FLAGS})

SET(PIRATE_LINK_LIBRARIES Threads::Threads)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(PIRATE_SHMEM_FEATURE)
SET(PIRATE_LINK_LIBRARIES ${PIRATE_LINK_LIBRARIES} rt)
endif(PIRATE_SHMEM_FEATURE)

target_link_libraries(primitives_test ${PIRATE_LINK_LIBRARIES})
target_link_libraries(primitives_bench_thr ${PIRATE_LINK_LIBRARIES})
target_link_libraries(primitives_bench_thr_vector ${PIRATE_LINK_LIBRARIES})
target_link_libraries(primitives_bench_lat ${PIRATE_LINK_LIBRARIES})
target_link_libraries(primitives_bench_lat_percentiles ${PIRATE_LINK_LIBRARIES})

enable_testing()

add_test(NAME primitives_test COMMAND primitives_test -v)

add_custom_target(testing COMMAND ${CMAKE_CTEST_COMMAND}
    --force-new-ctest-process
    --verbose
    --output-on-failure
)

 add_custom_target(valgrind COMMAND valgrind
    --leak-check=full
    --error-exitcode=1
    ./primitives_test
)
