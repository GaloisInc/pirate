cmake_minimum_required(VERSION 3.5)

project(camera_demo)

# Targets
set(TGT_CAMERA_DEMO camera_demo)

# Build flags
set(BUILD_FALGS -std=c++11 ${PIRATE_C_FLAGS})

# Libraries
find_library(FREESPACE_LIB freespace)
find_library(PIGPIO_LIB pigpio)
find_library(JPEG_LIB jpeg)
find_library(X11_LIB X11)
set(LIBS pthread)

# Source files
set(SRCS 
    src/main.cpp
    src/frameprocessor.cpp
    src/fileframeprocessor.cpp
    src/keyboardorientationinput.cpp
    src/orientationoutput.cpp
    src/videosensor.cpp
)

if (FREESPACE_LIB)
    set(BUILD_FALGS -DFREESPACE_PRESENT=1 ${BUILD_FALGS})
    set(LIBS ${FREESPACE_LIB} ${LIBS})
    set(SRCS ${SRCS} src/freespaceorientationinput.cpp)
endif (FREESPACE_LIB)

if (PIGPIO_LIB)
    set(BUILD_FALGS -DPIGPIO_PRESENT=1 ${BUILD_FALGS})
    set(LIBS ${PIGPIO_LIB} ${LIBS})
    set(SRCS ${SRCS} src/piservoorientationoutput.cpp)
endif (PIGPIO_LIB)

if (JPEG_LIB AND X11_LIB)
    set(BUILD_FALGS -DXWIN_PRESENT=1 ${BUILD_FALGS})
    set(LIBS ${JPEG_LIB} ${X11_LIB} ${LIBS})
    set(SRCS ${SRCS} src/xwinframeprocessor.cpp)
endif (JPEG_LIB AND X11_LIB)

add_executable(${TGT_CAMERA_DEMO} ${SRCS})
target_link_libraries(${TGT_CAMERA_DEMO} ${LIBS})
target_compile_options(${TGT_CAMERA_DEMO} PRIVATE ${BUILD_FALGS})

# Scripts
set(SCRIPTS_DIR ${PROJECT_SOURCE_DIR}/scripts)
configure_file(${SCRIPTS_DIR}/camera_demo.sh ${PROJECT_BINARY_DIR} COPYONLY)
