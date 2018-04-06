cmake_minimum_required(VERSION 3.0)

project(myi3status)

find_package(PkgConfig REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")

set(WITH_ALSA false CACHE BOOL "Enable ALSA support")
set(WITH_PULSE false CACHE BOOL "Enable PulseAudio support")
set(WITH_TIME false CACHE BOOL "Enable time support")
set(WITH_MEM false CACHE BOOL "Enable memory usage support")
set(WITH_POWER false CACHE BOOL "Enable power support (via procfs)")
set(WITH_NET false CACHE BOOL "Enable networking support (via netlink)")
set(WITH_NET_NL80211 false CACHE BOOL "Enable NL80211 extensions (via netlink)")

set(sources
    src/main.cpp
    src/cached_descriptors.cpp
    src/event_loop.cpp
    src/click_manager.cpp
    src/timer.cpp
    src/timer_oneshot.cpp
    src/signalfd.cpp
    src/utils.cpp
)

find_package(Boost)
include_directories(${Boost_INCLUDE_DIRS})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
list(APPEND libraries Threads::Threads)

pkg_search_module(YAJL REQUIRED yajl)

list(APPEND libraries ${YAJL_LIBRARIES})
include_directories(${YAJL_INCLUDE_DIR})

if (${WITH_ALSA})
    list(APPEND sources src/widget_alsa.cpp)
    find_package(ALSA)
    list(APPEND libraries ${ALSA_LIBRARY})
    include_directories(${ALSA_INCLUDE_DIR})
endif()

if (${WITH_PULSE})
    list(APPEND sources
        src/widget_pulse.cpp)
    find_library(LIBPULSE_LIBRARY NAMES pulse)
    list(APPEND libraries ${LIBPULSE_LIBRARY})
endif()

if (${WITH_TIME})
    list(APPEND sources
        src/widget_time.cpp)
endif()

if (${WITH_MEM})
    list(APPEND sources
        src/widget_mem.cpp)
endif()

if (${WITH_POWER})
    list(APPEND sources
        src/widget_ac.cpp
        src/widget_battery.cpp
        src/widget_wattage.cpp
        src/udev.cpp
    )
    list(APPEND libraries udev)
endif()

if (${WITH_NET} OR ${WITH_NET_NL80211})
    list(APPEND sources
        src/rtnetlink.cpp
        src/ext/genl.c
    )
    find_library(LIBNL_LIBRARY nl-3)

    find_path(LIBNL_INCLUDE_DIR
        NAMES netlink/netlink.h
        PATH_SUFFIXES libnl3
    )

    include_directories(${LIBNL_INCLUDE_DIR})

    find_library(LIBNL_GENL_LIBRARY nl-genl-3)
    find_library(LIBNL_ROUTE_LIBRARY nl-route-3)
    list(APPEND libraries
        ${LIBNL_LIBRARY} 
        ${LIBNL_GENL_LIBRARY}
        ${LIBNL_ROUTE_LIBRARY}
    )
endif()

if (${WITH_NET})
    list(APPEND sources
        src/widget_ip.cpp)
endif()

if (${WITH_NET_NL80211})
    list(APPEND sources
        src/widget_nl80211.cpp
        src/nl80211.cpp
        src/nl80211.c
    )
endif()

if (CMAKE_COMPILER_IS_GNUCC)
    add_definitions("-std=c++11 -fno-rtti -fno-exceptions")
endif()

add_executable(myi3status ${sources})
target_link_libraries(myi3status ${libraries})
