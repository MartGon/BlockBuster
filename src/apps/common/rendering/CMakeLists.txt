SET(SOURCES

    Camera.cpp
    Mesh.cpp
    Primitive.cpp
    Rendering.cpp
    TexturePalette.cpp
    ColorPalette.cpp
    ChunkMesh.cpp
    ChunkMeshMgr.cpp
)


add_library(Rendering ${SOURCES})
target_link_libraries(Rendering GLAD ${X11_LIBRARIES} ${OPENGL_LIBRARIES} ${CMAKE_DL_LIBS} ${SDL2_LIBRARIES} Threads::Threads GL Collisions)
target_include_directories(Rendering PUBLIC include ${OPENGL_INCLUDE_DIR} ${GLAD_INCLUDE_DIR} ${DEPS_FOLDER} ${CMAKE_CURRENT_SOURCE_DIR} ${APPS_COMMON_DIR} ${COMMON_DIR})