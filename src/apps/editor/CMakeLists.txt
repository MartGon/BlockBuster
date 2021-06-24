SET(SOURCES

    Editor.cpp
)

add_executable(MapEditor ${SOURCES})
target_link_libraries(MapEditor GLAD ${X11_LIBRARIES} ${GLFW_LIB_PATH} ${OPENGL_LIBRARIES} ${CMAKE_DL_LIBS} ${SDL2_LIBRARIES} Threads::Threads GL)
target_include_directories(MapEditor PUBLIC include ${OPENGL_INCLUDE_DIR} ${GLFW_INCLUDE_DIR} ${GLAD_INCLUDE_DIR} ${DEPS_FOLDER} ${COMMON_DIR})