cmake_minimum_required(VERSION 3.13)

project(cxx_demo)
set(CMAKE_BUILD_TYPE RelwithDebInfo)

# Dependencies
include_directories(../../libpirate)

# Build flags
SET(BUILD_FLAGS ${PIRATE_C_FLAGS})

SET(READER_TGT reader_cxx)
SET(WRITER_TGT writer_cxx)

ADD_EXECUTABLE(${READER_TGT} reader.cpp)
ADD_EXECUTABLE(${WRITER_TGT} writer.cpp)

TARGET_LINK_LIBRARIES(${READER_TGT} LINK_PUBLIC ${PIRATE_APP_LIBS})
TARGET_LINK_LIBRARIES(${WRITER_TGT} LINK_PUBLIC ${PIRATE_APP_LIBS})
