cmake_minimum_required(VERSION 3.20)

project(ffxivcrafting LANGUAGES CXX)
set(PROJECT_VERSION 0.1)

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE was not set: defaults to RelWithDebInfo")
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_subdirectory(vendor)
add_subdirectory(src)
