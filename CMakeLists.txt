cmake_minimum_required(VERSION 2.6)

project(myi3status)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")

file(GLOB_RECURSE sources src/*.cpp src/*.c)
file(GLOB_RECURSE headers src/*.h)

if (CMAKE_COMPILER_IS_GNUCC)
    add_definitions("-std=c++11")
endif()

add_executable(myi3status ${sources})

find_library(LIBNL_LIBRARY nl-3)
find_library(LIBNL_GENL_LIBRARY nl-genl-3)
find_library(LIBNL_ROUTE_LIBRARY nl-route-3)

find_path(LIBNL_INCLUDE_DIR
    NAMES
    netlink/netlink.h
    PATH_SUFFIXES
    libnl3
)

target_link_libraries(myi3status udev)
target_link_libraries(myi3status ${LIBNL_LIBRARY} ${LIBNL_GENL_LIBRARY} ${LIBNL_ROUTE_LIBRARY})

include_directories(${LIBNL_INCLUDE_DIR})
