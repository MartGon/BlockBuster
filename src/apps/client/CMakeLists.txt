SET(SOURCES

    Client.cpp
    Main.cpp

    GameState/GameState.cpp
    GameState/MainMenu/MainMenu.cpp
    GameState/MainMenu/MenuState.cpp

    GameState/InGame/InGame.cpp
    GameState/InGame/InGameGUI.cpp
    GameState/InGame/InGameNet.cpp
)

add_executable(Client ${SOURCES})
target_link_libraries(Client 
    imgui 
    GLAD 
    ${X11_LIBRARIES} 
    ${OPENGL_LIBRARIES}
    ${SDL2_LIBRARIES}
    Threads::Threads 
    Collisions 
    GL 
    Rendering
    Animation
    Math 
    Entity 
    App
    Game
    freetype
    GUI
    Audio
    OpenAL
    base64
    zip

    EnetW
    enet
    Networking
    HTTP
    nlohmann_json::nlohmann_json
)
target_include_directories(Client PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OPENGL_INCLUDE_DIR} 
    ${DEPS_FOLDER} 
    ${DEPS_FOLDER}/imgui
    ${DEPS_FOLDER}/enet/include
    ${DEPS_FOLDER}/json/include
    ${DEPS_FOLDER}/openal-soft/include
    ${COMMON_DIR} 
    ${APPS_COMMON_DIR}
)