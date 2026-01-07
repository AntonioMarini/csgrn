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

find_package(X11 REQUIRED)

# --- ImGui Configuration ---
set(IMGUI_DIR vendor/imgui)

# Create a static library for ImGui
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Allow targets linking 'imgui' to see the headers
target_include_directories(imgui PUBLIC 
    ${IMGUI_DIR} 
    ${IMGUI_DIR}/backends
)

# ImGui backends rely on GLFW and OpenGL
target_link_libraries(imgui PRIVATE glfw OpenGL::GL X11)

target_link_libraries(csgrn PRIVATE csgrn_lib glfw OpenGL::GL imgui)
