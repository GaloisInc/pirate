cmake_minimum_required(VERSION 3.5)

project(simple_demo)

include_directories(../libpirate ../tiny)

SET(CMAKE_C_COMPILER "clang")
SET(BUILD_FLAGS "-Werror" "-Wall" "-Wextra" "-Wpedantic")
SET(ELF_O_SUFFIX "elf.o")

set(HIGH_ELF "high")
add_executable(${HIGH_ELF} high.c)
target_link_libraries(${HIGH_ELF} pirateprims-static libtiny pthread)
target_compile_options(${HIGH_ELF} PRIVATE ${BUILD_FLAGS})

set(HIGH_ELF_O "${HIGH_ELF}_${ELF_O_SUFFIX}")
add_custom_target(HIGH_POST_BUILD
    COMMAND ld -r -b binary -o ${CMAKE_CURRENT_BINARY_DIR}/${HIGH_ELF_O} ${HIGH_ELF}
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/${HIGH_ELF_O}
    DEPENDS ${HIGH_ELF}
)

set(LOW_ELF "low")
add_executable(${LOW_ELF} low.c)
target_link_libraries(${LOW_ELF} pirateprims-static libtiny)
target_compile_options(${LOW_ELF} PRIVATE ${BUILD_FLAGS})

set(LOW_ELF_O "${LOW_ELF}_${ELF_O_SUFFIX}")
add_custom_target(LOW_POST_BUILD
    COMMAND ld -r -b binary -o ${CMAKE_CURRENT_BINARY_DIR}/${LOW_ELF_O} ${LOW_ELF}
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/${LOW_ELF_O}
    DEPENDS ${LOW_ELF}
)

set(DEMO_ELF "demo")
add_executable(${DEMO_ELF}
    gaps_wrapper.c 
    ${CMAKE_CURRENT_BINARY_DIR}/${HIGH_ELF_O}
    ${CMAKE_CURRENT_BINARY_DIR}/${LOW_ELF_O}
)
target_compile_options(${DEMO_ELF} PRIVATE ${BUILD_FLAGS})
add_dependencies(${DEMO_ELF} HIGH_POST_BUILD LOW_POST_BUILD)

set(STAIC_HTML "index.html")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${STAIC_HTML} ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${HIGH_ELF_O}
    ${CMAKE_CURRENT_BINARY_DIR}/${LOW_ELF_O}
    ${CMAKE_CURRENT_BINARY_DIR}/${STAIC_HTML}
)
