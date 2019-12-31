cmake_minimum_required(VERSION 3.5)
project(libpirate)

set(CMAKE_BUILD_TYPE RelwithDebInfo)
SET(PIRATE_SHMEM_FEATURE 0)

if(PIRATE_SHMEM_FEATURE)
add_definitions(-DPIRATE_SHMEM_FEATURE=1)
SET(PIRATE_SOURCES "primitives.c" "shmem_interface.c" "shmem.c" "tcp_socket.c" "udp_socket.c" "unix_socket.c" "uio.c")
else()
SET(PIRATE_SOURCES "primitives.c" "shmem_interface.c" "tcp_socket.c" "udp_socket.c" "unix_socket.c" "uio.c")
endif(PIRATE_SHMEM_FEATURE)

include_directories(. ../include)

add_executable(primitives_test ${PIRATE_SOURCES} test/primitives_test.c test/shmem_test.c test/uio_test.c)

add_executable(primitives_bench_thr ${PIRATE_SOURCES} bench/primitives_bench_thr.c)

add_executable(primitives_bench_thr_vector ${PIRATE_SOURCES} bench/primitives_bench_thr_vector.c)

add_executable(primitives_bench_lat ${PIRATE_SOURCES} bench/primitives_bench_lat.c)

add_executable(primitives_bench_lat_percentiles ${PIRATE_SOURCES} bench/primitives_bench_lat_percentiles.c)

add_library(pirateprims-static STATIC ${PIRATE_SOURCES})
set_target_properties(pirateprims-static PROPERTIES OUTPUT_NAME pirate CLEAN_DIRECT_OUTPUT 1)

add_library(pirateprims-shared SHARED ${PIRATE_SOURCES})
set_target_properties(pirateprims-shared PROPERTIES OUTPUT_NAME pirate CLEAN_DIRECT_OUTPUT 1)

target_compile_options(primitives_test PRIVATE -Werror -Wall -Wextra -Wpedantic)
target_compile_options(primitives_bench_thr PRIVATE -Werror -Wall -Wextra -Wpedantic)
target_compile_options(primitives_bench_thr_vector PRIVATE -Werror -Wall -Wextra -Wpedantic)
target_compile_options(primitives_bench_lat PRIVATE -Werror -Wall -Wextra -Wpedantic)
target_compile_options(primitives_bench_lat_percentiles PRIVATE -Werror -Wall -Wextra -Wpedantic)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(PIRATE_SHMEM_FEATURE)
target_link_libraries(primitives_test rt Threads::Threads)
target_link_libraries(primitives_bench_thr rt Threads::Threads)
target_link_libraries(primitives_bench_thr_vector rt Threads::Threads)
target_link_libraries(primitives_bench_lat rt Threads::Threads)
target_link_libraries(primitives_bench_lat_percentiles rt Threads::Threads)
else()
target_link_libraries(primitives_test Threads::Threads)
target_link_libraries(primitives_bench_thr Threads::Threads)
target_link_libraries(primitives_bench_thr_vector Threads::Threads)
target_link_libraries(primitives_bench_lat Threads::Threads)
target_link_libraries(primitives_bench_lat_percentiles Threads::Threads)
endif(PIRATE_SHMEM_FEATURE)

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
