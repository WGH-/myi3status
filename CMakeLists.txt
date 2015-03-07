cmake_minimum_required(VERSION 2.6)

project(myi3status)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")

file(GLOB_RECURSE sources src/*.cpp src/*.c)
file(GLOB_RECURSE headers src/*.h)

if (CMAKE_COMPILER_IS_GNUCC)
    add_definitions("-std=c++11")
endif()

add_executable(myi3status ${sources})

target_link_libraries(myi3status udev)
