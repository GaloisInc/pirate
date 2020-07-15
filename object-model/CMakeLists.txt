cmake_minimum_required(VERSION 3.10)

# set the project name
project(ObjectModel)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add the executable
add_executable(ObjectModel main.cpp Channel.cpp Channel.hpp)
target_link_libraries(ObjectModel pirate pal)

target_compile_options(ObjectModel PRIVATE "-ffunction-sections" "-fdata-sections")
set_target_properties(ObjectModel PROPERTIES LINK_FLAGS "${LINK_FLAGS} -Wl,-enclave,main -fuse-ld=lld")

add_executable(ObjectModelSend main.cpp Channel.cpp Channel.hpp message.hpp)
target_link_libraries(ObjectModelSend pirate pal)
target_compile_options(ObjectModelSend PRIVATE "-ffunction-sections" "-fdata-sections")
set_target_properties(ObjectModelSend PROPERTIES LINK_FLAGS "${LINK_FLAGS} -Wl,-enclave,send -fuse-ld=lld")

add_executable(ObjectModelRecv main.cpp Channel.cpp Channel.hpp message.hpp)
target_link_libraries(ObjectModelRecv pirate pal)
target_compile_options(ObjectModelRecv PRIVATE "-ffunction-sections" "-fdata-sections")
set_target_properties(ObjectModelRecv PROPERTIES LINK_FLAGS "${LINK_FLAGS} -Wl,-enclave,recv -fuse-ld=lld")
