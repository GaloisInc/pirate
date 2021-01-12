cmake_minimum_required(VERSION 3.5)

project(camera_demo)
set(CMAKE_BUILD_TYPE Debug)

# Targets
set(TGT_CAMERA_DEMO_GAPS camera_demo_gaps)
set(TGT_CAMERA_DEMO_MONOLITH camera_demo_monolith)

# Build flags
set(BUILD_FLAGS -std=c++11 -Wno-error=deprecated-declarations ${PIRATE_C_FLAGS})

# Libraries
find_library(FREESPACE_LIB freespace)
find_library(PIGPIO_LIB pigpio)
find_library(JPEG_LIB jpeg)
find_library(X11_LIB X11)
find_library(AVCODEC_LIB avcodec)
find_library(AVFORMAT_LIB avformat)
find_library(AVUTIL_LIB avutil)
find_library(SWSCALE_LIB swscale)
set(LIBS rt pthread pirate)

if (AVCODEC_LIB AND AVFORMAT_LIB AND AVUTIL_LIB AND SWSCALE_LIB)
    set(FFMPEG_LIBS 1)
endif (AVCODEC_LIB AND AVFORMAT_LIB AND AVUTIL_LIB AND SWSCALE_LIB)

# Source files
set(SRCS
    src/pantilt.cpp
    src/optionsparser.cpp
    src/imageconvert.cpp
    src/frameprocessor.cpp
    src/fileframeprocessor.cpp
    src/keyboardcameracontrolinput.cpp
    src/cameracontroloutput.cpp
    src/basecameracontroloutput.cpp
    src/remotecameracontroloutput.cpp
    src/testsource.cpp
    src/trilliumcontrol.cpp
    src/trilliumutilities.cpp
    src/videosource.cpp
    src/videosensor.cpp
    src/colortracking.cpp
    src/orion-sdk/fielddecode.cpp
    src/orion-sdk/fieldencode.cpp
    src/orion-sdk/floatspecial.cpp
    src/orion-sdk/KlvParser.cpp
    src/orion-sdk/KlvTree.cpp
    src/orion-sdk/OrionPublicPacket.cpp
    src/orion-sdk/OrionPublicPacketShim.cpp
    src/orion-sdk/scaleddecode.cpp
    src/orion-sdk/scaledencode.cpp
    src/orion-sdk/TrilliumPacket.cpp
)

if (FREESPACE_LIB)
    message("-- Freespace library found")
    set(BUILD_FLAGS -DFREESPACE_PRESENT=1 ${BUILD_FLAGS})
    set(LIBS ${FREESPACE_LIB} ${LIBS})
    set(SRCS ${SRCS} src/freespacecameracontrolinput.cpp)
else ()
    message("-- Freespace library not found")
endif (FREESPACE_LIB)

if (FFMPEG_LIBS)
    message("-- Ffmpeg libraries found")
    set(BUILD_FLAGS -DFFMPEG_PRESENT=1 ${BUILD_FLAGS})
    set(LIBS ${AVCODEC_LIB} ${AVFORMAT_LIB} ${AVUTIL_LIB} ${SWSCALE_LIB} ${LIBS})
    set(SRCS ${SRCS} src/mpeg-ts-encoder.cpp)
    set(SRCS ${SRCS} src/mpeg-ts-decoder.cpp)
    set(SRCS ${SRCS} src/trilliumvideosource.cpp)
else ()
    message("-- Ffmpeg libraries not found")
endif (FFMPEG_LIBS)

if (PIGPIO_LIB)
    message("-- Pigpio library found")
    set(BUILD_FLAGS -DPIGPIO_PRESENT=1 ${BUILD_FLAGS})
    set(LIBS ${PIGPIO_LIB} ${LIBS})
    set(SRCS ${SRCS} src/piservocameracontroloutput.cpp)
else ()
    message("-- Pigpio library not found")
endif (PIGPIO_LIB)

if (JPEG_LIB)
    message("-- Jpeg library found")
    set(BUILD_FLAGS -DJPEG_PRESENT=1 ${BUILD_FLAGS})
    set(LIBS ${JPEG_LIB} ${LIBS})
else ()
    message("-- Jpeg library not found")
endif (JPEG_LIB)

if (X11_LIB)
    message("-- X11 library found")
    set(BUILD_FLAGS -DXWIN_PRESENT=1 ${BUILD_FLAGS})
    set(LIBS ${X11_LIB} ${LIBS})
    set(SRCS ${SRCS} src/xwinframeprocessor.cpp src/metadataframeprocessor.cpp)
else ()
    message("-- X11 library not found")
endif (X11_LIB)

add_executable(${TGT_CAMERA_DEMO_GAPS} ${SRCS} src/main-gaps.cpp)
target_link_libraries(${TGT_CAMERA_DEMO_GAPS} ${LIBS})
target_compile_options(${TGT_CAMERA_DEMO_GAPS} PRIVATE ${BUILD_FLAGS})
target_include_directories(${TGT_CAMERA_DEMO_GAPS} PRIVATE src/orion-sdk)

add_executable(${TGT_CAMERA_DEMO_MONOLITH} ${SRCS} src/main-monolith.cpp)
target_link_libraries(${TGT_CAMERA_DEMO_MONOLITH} ${LIBS})
target_compile_options(${TGT_CAMERA_DEMO_MONOLITH} PRIVATE ${BUILD_FLAGS})
target_include_directories(${TGT_CAMERA_DEMO_MONOLITH} PRIVATE src/orion-sdk)

# Scripts
set(SCRIPTS_DIR ${PROJECT_SOURCE_DIR}/scripts)
configure_file(${SCRIPTS_DIR}/camera_demo.sh ${PROJECT_BINARY_DIR} COPYONLY)

# Configuration files
set(CONF_DIR ${PROJECT_SOURCE_DIR}/conf)
configure_file(${CONF_DIR}/trilliumconfig.txt ${PROJECT_BINARY_DIR} COPYONLY)
