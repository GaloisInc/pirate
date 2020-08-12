cmake_minimum_required(VERSION 3.5)

project(challenge_problems)
set(VERSION 0.0.2)

set(CMAKE_BUILD_TYPE Debug)

include(FindPackageHandleStandardArgs)

find_package(PkgConfig REQUIRED)

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

find_path(ACTIVEMQ_INCLUDE_DIR NAMES activemq/library/ActiveMQCPP.h PATH_SUFFIXES activemq-cpp-3.10.0)
find_library(ACTIVEMQ_LIBRARY NAMES activemq-cpp)
find_package_handle_standard_args(ACTIVEMQ DEFAULT_MSG ACTIVEMQ_LIBRARY ACTIVEMQ_INCLUDE_DIR)
if (ACTIVEMQ_FOUND)
    set(ACTIVEMQ_BUILD_FLAGS -Wno-deprecated-declarations ${BUILD_FLAGS})
    add_executable(challenge_pubsub_main challenge_pubsub_main.cpp ${COMMON_SRC})
    target_include_directories(challenge_pubsub_main PRIVATE ${ACTIVEMQ_INCLUDE_DIR})
    target_compile_options(challenge_pubsub_main PRIVATE ${ACTIVEMQ_BUILD_FLAGS})
    target_link_libraries(challenge_pubsub_main activemq-cpp)
    add_executable(challenge_pubsub_encrypt1 challenge_pubsub_encrypt1.cpp ${COMMON_SRC})
    target_include_directories(challenge_pubsub_encrypt1 PRIVATE ${ACTIVEMQ_INCLUDE_DIR})
    target_compile_options(challenge_pubsub_encrypt1 PRIVATE ${ACTIVEMQ_BUILD_FLAGS})
    target_link_libraries(challenge_pubsub_encrypt1 activemq-cpp)
    add_executable(challenge_pubsub_encrypt2 challenge_pubsub_encrypt2.cpp ${COMMON_SRC})
    target_include_directories(challenge_pubsub_encrypt2 PRIVATE ${ACTIVEMQ_INCLUDE_DIR})
    target_compile_options(challenge_pubsub_encrypt2 PRIVATE ${ACTIVEMQ_BUILD_FLAGS})
    target_link_libraries(challenge_pubsub_encrypt2 activemq-cpp)
endif (ACTIVEMQ_FOUND)

add_executable(challenge_multithreaded challenge_multithreaded.c ${COMMON_SRC})
target_compile_options(challenge_multithreaded PRIVATE ${BUILD_FLAGS})
target_link_libraries(challenge_multithreaded pthread)

pkg_check_modules(PROTOBUF_PKG IMPORTED_TARGET protobuf)
pkg_check_modules(GRPC_PKG IMPORTED_TARGET grpc++)
if (PROTOBUF_PKG_FOUND AND GRPC_PKG_FOUND)
    add_executable(challenge_rpc_main challenge_rpc_main.cpp challenge_rpc.grpc.pb.cc ${COMMON_SRC})
    target_compile_options(challenge_rpc_main PRIVATE ${BUILD_FLAGS})
    target_link_libraries(challenge_rpc_main PkgConfig::PROTOBUF_PKG PkgConfig::GRPC_PKG pthread)
    add_executable(challenge_rpc_encrypt1 challenge_rpc_encrypt1.cpp challenge_rpc.grpc.pb.cc ${COMMON_SRC})
    target_compile_options(challenge_rpc_encrypt1 PRIVATE ${BUILD_FLAGS})
    target_link_libraries(challenge_rpc_encrypt1 PkgConfig::PROTOBUF_PKG PkgConfig::GRPC_PKG pthread)
    add_executable(challenge_rpc_encrypt2 challenge_rpc_encrypt2.cpp challenge_rpc.grpc.pb.cc ${COMMON_SRC})
    target_compile_options(challenge_rpc_encrypt2 PRIVATE ${BUILD_FLAGS})
    target_link_libraries(challenge_rpc_encrypt2 PkgConfig::PROTOBUF_PKG PkgConfig::GRPC_PKG pthread)
endif (PROTOBUF_PKG_FOUND AND GRPC_PKG_FOUND)

find_path(ASYNCXX_INCLUDE_DIR NAMES async++.h)
find_library(ASYNCXX_LIBRARY NAMES async++)
find_package_handle_standard_args(ASYNCXX DEFAULT_MSG ASYNCXX_LIBRARY ASYNCXX_INCLUDE_DIR)
if (ASYNCXX_FOUND)
    add_executable(challenge_async challenge_async.cpp ${COMMON_SRC})
    target_include_directories(challenge_async PRIVATE ${ASYNCXX_INCLUDE_DIR})
    target_compile_options(challenge_async PRIVATE ${BUILD_FLAGS})
    target_link_libraries(challenge_async async++)
endif (ASYNCXX_FOUND)

add_executable(challenge_spaghetti challenge_spaghetti.c ${COMMON_SRC})
target_compile_options(challenge_spaghetti PRIVATE ${BUILD_FLAGS})
