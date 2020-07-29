cmake_minimum_required(VERSION 3.5)

project(challenge_problems)
set(VERSION 0.0.2)

set(CMAKE_BUILD_TYPE Debug)

# Build flags
set(BUILD_FLAGS -Werror -Wall -Wextra -Wpedantic)

set(COMMON_SRC base64.c tweetnacl.c)

add_executable(challenge_baseline_encrypt challenge_baseline_encrypt.c ${COMMON_SRC})
target_compile_options(challenge_baseline_encrypt PRIVATE ${BUILD_FLAGS})

add_executable(challenge_baseline_decrypt challenge_baseline_decrypt.c ${COMMON_SRC})
target_compile_options(challenge_baseline_decrypt PRIVATE ${BUILD_FLAGS})

add_executable(challenge_socket_main challenge_socket_main.c challenge_socket_common.c ${COMMON_SRC})
target_compile_options(challenge_socket_main PRIVATE ${BUILD_FLAGS})
add_executable(challenge_socket_encrypt1 challenge_socket_encrypt1.c challenge_socket_common.c ${COMMON_SRC})
target_compile_options(challenge_socket_encrypt1 PRIVATE ${BUILD_FLAGS})
add_executable(challenge_socket_encrypt2 challenge_socket_encrypt2.c challenge_socket_common.c ${COMMON_SRC})
target_compile_options(challenge_socket_encrypt2 PRIVATE ${BUILD_FLAGS})

set(ACTIVEMQ_BUILD_FLAGS -Wno-deprecated-declarations -I/usr/local/include/activemq-cpp-3.10.0 ${BUILD_FLAGS})
add_executable(challenge_pubsub_main challenge_pubsub_main.cpp ${COMMON_SRC})
target_compile_options(challenge_pubsub_main PRIVATE ${ACTIVEMQ_BUILD_FLAGS})
target_link_libraries(challenge_pubsub_main activemq-cpp)
add_executable(challenge_pubsub_encrypt1 challenge_pubsub_encrypt1.cpp ${COMMON_SRC})
target_compile_options(challenge_pubsub_encrypt1 PRIVATE ${ACTIVEMQ_BUILD_FLAGS})
target_link_libraries(challenge_pubsub_encrypt1 activemq-cpp)
add_executable(challenge_pubsub_encrypt2 challenge_pubsub_encrypt2.cpp ${COMMON_SRC})
target_compile_options(challenge_pubsub_encrypt2 PRIVATE ${ACTIVEMQ_BUILD_FLAGS})
target_link_libraries(challenge_pubsub_encrypt2 activemq-cpp)

add_executable(challenge_multithreaded challenge_multithreaded.c ${COMMON_SRC})
target_compile_options(challenge_multithreaded PRIVATE ${BUILD_FLAGS})
target_link_libraries(challenge_multithreaded pthread)

add_executable(challenge_spaghetti challenge_spaghetti.c ${COMMON_SRC})
target_compile_options(challenge_spaghetti PRIVATE ${BUILD_FLAGS})
