SET(SOURCES

    BlockBuster.cpp
)

add_executable(BlockBuster ${SOURCES})
target_link_libraries(BlockBuster imgui GLAD ${X11_LIBRARIES} ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} Threads::Threads Collisions GL Rendering Math)
target_include_directories(BlockBuster PUBLIC ${OPENGL_INCLUDE_DIR} ${DEPS_FOLDER} ${DEPS_FOLDER}/imgui ${COMMON_DIR} ${APPS_COMMON_DIR})
message("This is gl ${GL}")