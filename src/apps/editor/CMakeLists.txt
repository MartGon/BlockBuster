SET(SOURCES

    Editor.cpp
    BlockBuster.cpp
    ToolAction.cpp
)

add_executable(Editor ${SOURCES})
target_link_libraries(Editor 
    imgui 
    GLAD 
    ${X11_LIBRARIES} 
    ${OPENGL_LIBRARIES}
    ${SDL2_LIBRARIES}
    Threads::Threads 
    Collisions 
    GL 
    Rendering 
    Math 
    Game 
    App
    AppsGame
    
    Debug
)
target_include_directories(Editor PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OPENGL_INCLUDE_DIR} 
    ${DEPS_FOLDER} 
    ${DEPS_FOLDER}/imgui 
    ${COMMON_DIR} 
    ${APPS_COMMON_DIR}
)