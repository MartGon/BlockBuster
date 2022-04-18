find_package(OpenGL REQUIRED)
find_package(X11 REQUIRED)
find_package(SDL2 REQUIRED)

add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(editor)