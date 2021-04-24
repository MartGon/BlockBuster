# Set minimum CMake version
cmake_minimum_required(VERSION 3.1...3.16)

# Set project name
project(OpenGLChess VERSION 1.0 LANGUAGES C CXX)

# Set C++ version
set (CMAKE_CXX_STANDARD 17)

set(DEPS_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/deps)
set(SHADERS_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

add_compile_definitions(SHADERS_DIR="${SHADERS_FOLDER}")
add_compile_definitions(TEXTURES_DIR="${CMAKE_SOURCE_DIR}/resources/textures/")

add_subdirectory(deps)
add_subdirectory(src)
