cmake_minimum_required(VERSION 3.5)
project(libpirate)

set(CMAKE_BUILD_TYPE RelwithDebInfo)

SET(PIRATE_SOURCES
    "primitives.c"
    "pirate_common.c"
    "device.c"
    "pipe.c"
    "ge_eth.c"
    "mercury.c"
    "mercury_cntl.c"
    "serial.c"
    "shmem_interface.c"
    "udp_shmem_interface.c"
    "tcp_socket.c"
    "udp_socket.c"
    "unix_socket.c"
    "uio_interface.c"
)

if(PIRATE_SHMEM_FEATURE)
    add_definitions(-DPIRATE_SHMEM_FEATURE=1)
    set(PIRATE_SOURCES ${PIRATE_SOURCES} "shmem.c" "uio.c" "udp_shmem.c" "checksum.c")
endif(PIRATE_SHMEM_FEATURE)

include_directories(
    .
    ${PROJECT_SOURCE_DIR}/../devices/mercury/loopback_ilip/include
)

add_library(${PIRATE_STATIC_LIB} STATIC ${PIRATE_SOURCES})
set_target_properties(${PIRATE_STATIC_LIB} PROPERTIES OUTPUT_NAME pirate CLEAN_DIRECT_OUTPUT 1)
target_compile_options(${PIRATE_STATIC_LIB} PRIVATE ${PIRATE_C_FLAGS})

add_library(${PIRATE_SHARED_LIB} SHARED ${PIRATE_SOURCES})
set_target_properties(${PIRATE_SHARED_LIB} PROPERTIES OUTPUT_NAME pirate CLEAN_DIRECT_OUTPUT 1)
target_compile_options(${PIRATE_SHARED_LIB} PRIVATE ${PIRATE_C_FLAGS})

install(FILES libpirate.h shmem_buffer.h DESTINATION include)
install(TARGETS ${PIRATE_SHARED_LIB} DESTINATION lib)
install(TARGETS ${PIRATE_STATIC_LIB} DESTINATION lib)

if(GAPS_BENCH)
    include_directories(BEFORE libpirate)
    link_directories(BEFORE ${CMAKE_BINARY_DIR}/libpirate)

    add_executable(primitives_bench_thr ${PIRATE_SOURCES} bench/primitives_bench_thr.c)
    add_executable(primitives_bench_thr_vector ${PIRATE_SOURCES} bench/primitives_bench_thr_vector.c)
    add_executable(primitives_bench_lat ${PIRATE_SOURCES} bench/primitives_bench_lat.c)
    add_executable(primitives_bench_lat_percentiles ${PIRATE_SOURCES} bench/primitives_bench_lat_percentiles.c)
    set_target_properties(primitives_bench_thr primitives_bench_thr_vector primitives_bench_lat primitives_bench_lat_percentiles PROPERTIES EXCLUDE_FROM_ALL 1)
    add_custom_target(bench DEPENDS primitives_bench_thr primitives_bench_thr_vector primitives_bench_lat primitives_bench_lat_percentiles)

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

    target_link_libraries(primitives_bench_thr ${PIRATE_LINK_LIBRARIES})
    target_link_libraries(primitives_bench_thr_vector ${PIRATE_LINK_LIBRARIES})
    target_link_libraries(primitives_bench_lat ${PIRATE_LINK_LIBRARIES})
    target_link_libraries(primitives_bench_lat_percentiles ${PIRATE_LINK_LIBRARIES})
endif(GAPS_BENCH)

# GAPS channel unit test
if(PIRATE_UNIT_TEST)
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIR})

    include_directories(BEFORE libpirate)
    link_directories(BEFORE ${CMAKE_BINARY_DIR}/libpirate)

    set(PIRATE_C_TEST_FLAGS ${PIRATE_C_FLAGS})
    set(GAPS_CHANNELS_TEST_LIBS ${PIRATE_APP_LIBS} pthread)
    set(GAPS_CHANNELS_TEST gaps_channels_test)
    file(GLOB GAPS_CHANNELS_TEST_SRC test/*.cpp)
    add_executable(${GAPS_CHANNELS_TEST} ${GAPS_CHANNELS_TEST_SRC})
    target_compile_options(${GAPS_CHANNELS_TEST} PRIVATE ${PIRATE_C_TEST_FLAGS})

    # Link test executable against gtest & gtest_main
    target_link_libraries(${GAPS_CHANNELS_TEST}
        ${GTEST_MAIN_LIBRARY}
        ${GTEST_LIBRARIES}
        ${GAPS_CHANNELS_TEST_LIBS}
    )

    enable_testing()

    add_custom_target(testing COMMAND ${CMAKE_CTEST_COMMAND}
        --force-new-ctest-process
        --verbose
        --output-on-failure
    )

     add_custom_target(valgrind COMMAND valgrind
        --leak-check=full
        --error-exitcode=1
        ./${GAPS_CHANNELS_TEST}
    )

    add_test(
        NAME ${GAPS_CHANNELS_TEST}
        COMMAND ${GAPS_CHANNELS_TEST}
    )
endif(PIRATE_UNIT_TEST)
