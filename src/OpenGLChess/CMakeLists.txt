SET(SOURCES

    src/OpenGLChess.cpp
)

find_package(OpenGL REQUIRED)
find_package(X11 REQUIRED)
find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED)

add_executable(OpenGLChess ${SOURCES})
target_link_libraries(OpenGLChess GLAD ${X11_LIBRARIES} ${GLFW_LIB_PATH} ${OPENGL_LIBRARIES} ${CMAKE_DL_LIBS} ${SDL2_LIBRARIES} Threads::Threads)
target_include_directories(OpenGLChess PUBLIC include ${OPENGL_INCLUDE_DIR} ${GLFW_INCLUDE_DIR} ${GLAD_INCLUDE_DIR} ${DEPS_FOLDER} ${SDL2_INCLUDE_DIRS})
message("Include dir is ${GLAD_INCLUDE_DIR}")