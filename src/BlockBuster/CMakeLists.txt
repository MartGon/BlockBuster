SET(SOURCES

    BlockBuster.cpp
)

add_executable(BlockBuster ${SOURCES})
target_link_libraries(BlockBuster GLAD ${X11_LIBRARIES} ${GLFW_LIB_PATH} ${OPENGL_LIBRARIES} ${CMAKE_DL_LIBS} ${SDL2_LIBRARIES} Threads::Threads GL)
target_include_directories(BlockBuster PUBLIC include ${OPENGL_INCLUDE_DIR} ${GLFW_INCLUDE_DIR} ${GLAD_INCLUDE_DIR} ${DEPS_FOLDER} ${CMAKE_CURRENT_SOURCE_DIR} ..)
message("Include dir is ${GLAD_INCLUDE_DIR}")