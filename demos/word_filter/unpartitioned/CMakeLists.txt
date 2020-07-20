cmake_minimum_required(VERSION 3.5)

project(word_filter)
set(CMAKE_BUILD_TYPE RelwithDebInfo)

# Build flags
set(BUILD_FLAGS "-Werror" "-Wall" "-Wextra" "-Wpedantic" "-O0")
set(BUILD_FLAGS ${BUILD_FLAGS} "-ffunction-sections" "-fdata-sections" "--target=x86_64-pc-linux-elf")

add_executable("word_filter" "filter.c")
