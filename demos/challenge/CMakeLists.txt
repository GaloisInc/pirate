cmake_minimum_required(VERSION 3.5)

project(challenge_problems)
set(VERSION 0.0.2)

set(CMAKE_BUILD_TYPE Debug)

# Build flags
set(BUILD_FLAGS -Werror -Wall -Wextra -Wpedantic)

set(COMMON_SRC base64.c tweetnacl.c )

add_executable(challenge_baseline challenge_baseline.c ${COMMON_SRC})
target_compile_options(challenge_baseline PRIVATE ${BUILD_FLAGS})

add_executable(challenge_spaghetti challenge_spaghetti.c ${COMMON_SRC})
target_compile_options(challenge_spaghetti PRIVATE ${BUILD_FLAGS})

add_executable(challenge_multithreaded challenge_multithreaded.c ${COMMON_SRC})
target_compile_options(challenge_multithreaded PRIVATE ${BUILD_FLAGS})
target_link_libraries(challenge_multithreaded pthread)
