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

ADD_EXECUTABLE(reader reader.cpp)
ADD_EXECUTABLE(writer writer.cpp)

TARGET_LINK_LIBRARIES(reader LINK_PUBLIC ${LIBS})
TARGET_LINK_LIBRARIES(writer LINK_PUBLIC ${LIBS})
