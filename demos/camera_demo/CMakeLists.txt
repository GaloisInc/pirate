cmake_minimum_required(VERSION 3.5)

project(camera_demo)

# Targets
set(TGT_CAMERA_DEMO camera_demo)

# Libraries
find_library(FREESPACE_LIB freespace)
find_library(PIGPIO_LIB pigpio)

file(GLOB SRCS src/*.cpp)

add_executable(${TGT_CAMERA_DEMO} ${SRCS})
target_link_libraries(${TGT_CAMERA_DEMO} ${FREESPACE_LIB} ${PIGPIO_LIB} pthread)
target_compile_options(${TGT_CAMERA_DEMO} PRIVATE ${BUILD_FLAGS})

