# Set minimum CMake version
cmake_minimum_required(VERSION 3.1...3.16)

# Set project name
project(BlockBuster VERSION 1.0 LANGUAGES C CXX)

# Set C++ version
set (CMAKE_CXX_STANDARD 17)

set(DEPS_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/deps)
set(SHADERS_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

add_compile_definitions(SHADERS_DIR="${SHADERS_FOLDER}")
add_compile_definitions(TEXTURES_DIR="${CMAKE_SOURCE_DIR}/resources/textures/")
add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLAD=1)
add_compile_definitions(NOMINMAX=1)
# Add compile defines
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_compile_definitions(_DEBUG=1)
    add_compile_definitions(_LOG=1)
endif()

find_package(OpenGL REQUIRED)
find_package(X11 REQUIRED)
find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED)

add_subdirectory(deps)
add_subdirectory(src)

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
