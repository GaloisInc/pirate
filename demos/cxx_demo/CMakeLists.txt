cmake_minimum_required(VERSION 3.13)

project(cxx_demo)
set(CMAKE_BUILD_TYPE RelwithDebInfo)

# Options
if (GAPS_ENABLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGAPS_ENABLE")
endif(GAPS_ENABLE)

# Dependencies
include_directories(../../libpirate)
SET(LIBS "pirateprims-static")

# Build flags
set(BUILD_FLAGS "-Werror" "-Wall" "-Wextra" "-Wpedantic" "-O3")
if (GAPS_ENABLE)
    set(BUILD_FLAGS ${BUILD_FLAGS} "-ffunction-sections" "-fdata-sections" "--target=x86_64-pc-linux-elf")
endif(GAPS_ENABLE)

SET(READER_TGT reader_cxx)
SET(WRITER_TGT writer_cxx)

ADD_EXECUTABLE(${READER_TGT} reader.cpp)
ADD_EXECUTABLE(${WRITER_TGT} writer.cpp)

TARGET_LINK_LIBRARIES(${READER_TGT} LINK_PUBLIC ${LIBS})
TARGET_LINK_LIBRARIES(${WRITER_TGT} LINK_PUBLIC ${LIBS})
