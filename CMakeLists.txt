cmake_minimum_required(VERSION 3.20)
project(csgrn)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(csgrn_lib INTERFACE)
target_include_directories(csgrn_lib INTERFACE 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

add_executable(csgrn
    src/main.cpp
    src/glad.c
)

file(COPY ${CMAKE_SOURCE_DIR}/src/shaders 
     DESTINATION ${CMAKE_BINARY_DIR})

add_subdirectory(vendor/glfw)

find_package(OpenGL REQUIRED)

target_link_libraries(csgrn PRIVATE csgrn_lib glfw OpenGL::GL)
