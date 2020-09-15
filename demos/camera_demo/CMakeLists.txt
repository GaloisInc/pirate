cmake_minimum_required(VERSION 3.5)

project(camera_demo)
set(CMAKE_BUILD_TYPE Debug)

# Targets
set(TGT_CAMERA_DEMO camera_demo)

# Build flags
set(BUILD_FALGS -std=c++11 -Wno-error=deprecated-declarations ${PIRATE_C_FLAGS})

# Libraries
find_library(FREESPACE_LIB freespace)
find_library(PIGPIO_LIB pigpio)
find_library(JPEG_LIB jpeg)
find_library(X11_LIB X11)
find_library(AVCODEC_LIB avcodec)
find_library(AVFORMAT_LIB avformat)
find_library(AVUTIL_LIB avutil)
find_library(SWSCALE_LIB swscale)
set(LIBS pthread)

if (AVCODEC_LIB AND AVFORMAT_LIB AND AVUTIL_LIB AND SWSCALE_LIB)
    set(FFMPEG_LIBS 1)
endif (AVCODEC_LIB AND AVFORMAT_LIB AND AVUTIL_LIB AND SWSCALE_LIB)

# Source files
set(SRCS 
    src/main.cpp
    src/imageconvert.cpp
    src/frameprocessor.cpp
    src/fileframeprocessor.cpp
    src/keyboardorientationinput.cpp
    src/orientationoutput.cpp
    src/videosensor.cpp
    src/colortracking.cpp
)

if (FREESPACE_LIB)
    message("-- Freespace library found")
    set(BUILD_FALGS -DFREESPACE_PRESENT=1 ${BUILD_FALGS})
    set(LIBS ${FREESPACE_LIB} ${LIBS})
    set(SRCS ${SRCS} src/freespaceorientationinput.cpp)
else ()
    message("-- Freespace library not found")
endif (FREESPACE_LIB)

if (FFMPEG_LIBS)
    message("-- Ffmpeg libraries found")
    set(BUILD_FALGS -DFFMPEG_PRESENT=1 ${BUILD_FALGS})
    set(LIBS ${AVCODEC_LIB} ${AVFORMAT_LIB} ${AVUTIL_LIB} ${SWSCALE_LIB} ${LIBS})
    set(SRCS ${SRCS} src/h264encoder.cpp)
else ()
    message("-- Ffmpeg libraries not found")
endif (FFMPEG_LIBS)

if (PIGPIO_LIB)
    message("-- Pigpio library found")
    set(BUILD_FALGS -DPIGPIO_PRESENT=1 ${BUILD_FALGS})
    set(LIBS ${PIGPIO_LIB} ${LIBS})
    set(SRCS ${SRCS} src/piservoorientationoutput.cpp)
else ()
    message("-- Pigpio library not found")
endif (PIGPIO_LIB)

if (JPEG_LIB AND X11_LIB)
    message("-- Jpeg and X11 libraries found")
    set(BUILD_FALGS -DXWIN_PRESENT=1 ${BUILD_FALGS})
    set(LIBS ${JPEG_LIB} ${X11_LIB} ${LIBS})
    set(SRCS ${SRCS} src/xwinframeprocessor.cpp)
else ()
    message("-- Jpeg and X11 libraries not found")
endif (JPEG_LIB AND X11_LIB)

add_executable(${TGT_CAMERA_DEMO} ${SRCS})
target_link_libraries(${TGT_CAMERA_DEMO} ${LIBS})
target_compile_options(${TGT_CAMERA_DEMO} PRIVATE ${BUILD_FALGS})

# Scripts
set(SCRIPTS_DIR ${PROJECT_SOURCE_DIR}/scripts)
configure_file(${SCRIPTS_DIR}/camera_demo.sh ${PROJECT_BINARY_DIR} COPYONLY)
