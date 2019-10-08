cmake_minimum_required(VERSION 3.5)
project(libpirate)

set(CMAKE_BUILD_TYPE RelwithDebInfo)

include_directories(. ../include)

add_executable(primitives_test primitives.c test/primitives_test.c)

add_executable(primitives_bench primitives.c bench/primitives_bench.c)

add_library(pirateprims-static STATIC primitives.c)
set_target_properties(pirateprims-static PROPERTIES OUTPUT_NAME pirate CLEAN_DIRECT_OUTPUT 1)

add_library(pirateprims-shared SHARED primitives.c)
set_target_properties(pirateprims-shared PROPERTIES OUTPUT_NAME pirate CLEAN_DIRECT_OUTPUT 1)

target_compile_options(primitives_test PRIVATE -Werror -Wall -Wextra -Wpedantic)
target_compile_options(primitives_bench PRIVATE -Werror -Wall -Wextra -Wpedantic)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(primitives_test Threads::Threads)

enable_testing()

add_test(NAME primitives_test COMMAND primitives_test -v)

add_custom_target(testing COMMAND ${CMAKE_CTEST_COMMAND}
    --force-new-ctest-process
    --verbose
    --output-on-failure
)
