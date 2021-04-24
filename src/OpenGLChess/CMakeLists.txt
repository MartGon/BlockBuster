SET(SOURCES

    OpenGLChess.cpp
)

add_executable(OpenGLChess ${SOURCES})
target_link_libraries(OpenGLChess GLAD ${X11_LIBRARIES} ${GLFW_LIB_PATH} ${OPENGL_LIBRARIES} ${CMAKE_DL_LIBS} ${SDL2_LIBRARIES} Threads::Threads GL)
target_include_directories(OpenGLChess PUBLIC include ${OPENGL_INCLUDE_DIR} ${GLFW_INCLUDE_DIR} ${GLAD_INCLUDE_DIR} ${DEPS_FOLDER} ${CMAKE_CURRENT_SOURCE_DIR} ../GL)
message("Include dir is ${GLAD_INCLUDE_DIR}")