cmake_minimum_required(VERSION 3.5)
project(libpirate)

set(CMAKE_BUILD_TYPE RelwithDebInfo)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    SET(PIRATE_SOURCES
        "windows/primitives.c"
        "windows/pirate_common.c"
        "windows/udp_socket.c"
        "windows/ge_eth.c"
    )
else()
    SET(PIRATE_SOURCES
        "primitives.c"
        "pirate_common.c"
        "device.c"
        "pipe.c"
        "ge_eth.c"
        "mercury.c"
        "mercury_cntl.c"
        "serial.c"
        "tcp_socket.c"
        "udp_socket.c"
        "unix_socket.c"
        "unix_seqpacket.c"
    )

    if(PIRATE_SHMEM_FEATURE)
        add_definitions(-DPIRATE_SHMEM_FEATURE=1)
        set(PIRATE_SOURCES ${PIRATE_SOURCES} "shmem.c" "uio.c" "udp_shmem.c" "checksum.c")
    endif(PIRATE_SHMEM_FEATURE)
endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

include_directories(
    .
    include
    ${PROJECT_SOURCE_DIR}/../devices/mercury/loopback_ilip/include
)

add_library(${PIRATE_STATIC_LIB} STATIC ${PIRATE_SOURCES})
target_include_directories(${PIRATE_STATIC_LIB} PUBLIC include)
set_target_properties(${PIRATE_STATIC_LIB} PROPERTIES OUTPUT_NAME pirate CLEAN_DIRECT_OUTPUT 1)
target_compile_options(${PIRATE_STATIC_LIB} PRIVATE ${PIRATE_C_FLAGS})

add_library(${PIRATE_SHARED_LIB} SHARED ${PIRATE_SOURCES})
set_target_properties(${PIRATE_SHARED_LIB} PROPERTIES OUTPUT_NAME pirate CLEAN_DIRECT_OUTPUT 1)
target_compile_options(${PIRATE_SHARED_LIB} PRIVATE ${PIRATE_C_FLAGS})
target_include_directories(${PIRATE_SHARED_LIB} PUBLIC include)


install(FILES include/libpirate.h include/libpirate.hpp shmem_buffer.h DESTINATION include)
install(TARGETS ${PIRATE_SHARED_LIB} DESTINATION lib)
install(TARGETS ${PIRATE_STATIC_LIB} DESTINATION lib)

if(GAPS_BENCH)
    include_directories(BEFORE libpirate)
    link_directories(BEFORE ${CMAKE_BINARY_DIR}/libpirate)

    add_executable(bench_thr_reader bench/bench_thr_reader.c bench/bench_thr_common.c)
    add_executable(bench_thr_writer bench/bench_thr_writer.c bench/bench_thr_common.c)
    add_executable(bench_lat1 bench/bench_lat1.c bench/bench_lat_common.c)
    add_executable(bench_lat2 bench/bench_lat2.c bench/bench_lat_common.c)

    target_compile_options(bench_thr_reader PRIVATE ${PIRATE_C_FLAGS})
    target_compile_options(bench_thr_writer PRIVATE ${PIRATE_C_FLAGS})
    target_compile_options(bench_lat1 PRIVATE ${PIRATE_C_FLAGS})
    target_compile_options(bench_lat2 PRIVATE ${PIRATE_C_FLAGS})

    target_link_libraries(bench_thr_reader ${PIRATE_APP_LIBS})
    target_link_libraries(bench_thr_writer ${PIRATE_APP_LIBS})
    target_link_libraries(bench_lat1 ${PIRATE_APP_LIBS})
    target_link_libraries(bench_lat2 ${PIRATE_APP_LIBS})

    configure_file(bench/bench.py ${PROJECT_BINARY_DIR} COPYONLY)
endif(GAPS_BENCH)

# GAPS channel unit test
if(PIRATE_UNIT_TEST)
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIR})

    include_directories(BEFORE libpirate)
    link_directories(BEFORE ${CMAKE_BINARY_DIR}/libpirate)

    if(NOT IPV6_TESTS)
        add_definitions(-DDISABLE_IPV6_TESTS)
    endif(NOT IPV6_TESTS)

    set(PIRATE_C_TEST_FLAGS ${PIRATE_C_FLAGS})
    set(GAPS_CHANNELS_TEST gaps_channels_test)
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(GAPS_CHANNELS_TEST_LIBS ${PIRATE_APP_LIBS})
    SET(GAPS_CHANNELS_TEST_SRC
        "test/channel_test.cpp"
        "test/udp_socket_test.cpp"
        "test/ge_eth_test.cpp")
else()
    set(GAPS_CHANNELS_TEST_LIBS ${PIRATE_APP_LIBS} pthread)
    if (PIRATE_SHMEM_FEATURE)
      set(GAPS_CHANNELS_TEST_LIBS ${GAPS_CHANNELS_TEST_LIBS} rt)
    endif(PIRATE_SHMEM_FEATURE)
    file(GLOB GAPS_CHANNELS_TEST_SRC test/*.cpp)
endif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_executable(${GAPS_CHANNELS_TEST} ${GAPS_CHANNELS_TEST_SRC})
    target_compile_options(${GAPS_CHANNELS_TEST} PRIVATE ${PIRATE_C_TEST_FLAGS})

    # Link test executable against gtest & gtest_main
    target_link_libraries(${GAPS_CHANNELS_TEST}
        ${GTEST_MAIN_LIBRARY}
        ${GTEST_LIBRARIES}
        ${GAPS_CHANNELS_TEST_LIBS}
    )

    add_custom_target(valgrind COMMAND valgrind
        --leak-check=full
        --error-exitcode=1
        ./${GAPS_CHANNELS_TEST} --gtest_filter=-*MercuryFunctionalTest*
    )
endif(PIRATE_UNIT_TEST)

install(TARGETS ${PIRATE_SHARED_LIB} DESTINATION lib)
install(TARGETS ${PIRATE_STATIC_LIB} DESTINATION lib)
install(FILES include/libpirate.h include/libpirate.hpp DESTINATION include)
