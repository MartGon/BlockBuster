SET(SOURCES

    BlockBuster.cpp
)

add_executable(BlockBuster ${SOURCES})
target_link_libraries(BlockBuster GLAD ${X11_LIBRARIES} ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} Threads::Threads GL)
target_include_directories(BlockBuster PUBLIC ${OPENGL_INCLUDE_DIR} ${DEPS_FOLDER} ${COMMON_DIR})