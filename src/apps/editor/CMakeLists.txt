SET(SOURCES

    Main.cpp
    Editor.cpp
    EditorGUI.cpp
    Project.cpp
    ToolAction.cpp
)

add_executable(Editor ${SOURCES})
target_link_libraries(Editor 
    GLAD 
    imgui 
    ${X11_LIBRARIES} 
    ${OPENGL_LIBRARIES} 
    ${CMAKE_DL_LIBS} 
    ${SDL2_LIBRARIES} 
    Threads::Threads 
    Collisions 
    GL 
    Rendering 
    Math 
    BlockBuster 
    App
    Game
    
    Debug
)
target_include_directories(Editor PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OPENGL_INCLUDE_DIR} 
    ${DEPS_FOLDER} 
    ${DEPS_FOLDER}/imgui 
    ${COMMON_DIR} 
    ${APPS_COMMON_DIR}
    ${SDL2_INCLUDE_DIRS}
)