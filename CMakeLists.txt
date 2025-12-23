cmake_minimum_required(VERSION 3.20)
project(csgrn)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(csgrn
    src/main.cpp
    src/glad.c
)

file(COPY ${CMAKE_SOURCE_DIR}/src/shaders 
     DESTINATION ${CMAKE_BINARY_DIR})

add_subdirectory(vendor/glfw)

target_include_directories(csgrn PRIVATE include)
find_package(OpenGL REQUIRED)

target_link_libraries(csgrn PRIVATE glfw OpenGL::GL)

