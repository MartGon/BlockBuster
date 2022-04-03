SET(SOURCES
    Block.cpp
    Map.cpp
    Game.cpp
    GameMode.cpp
    Player.cpp
    PlayerController.cpp
    Weapon.cpp
    GameObject.cpp
    Match.cpp
    Scoreboard.cpp
)


add_library(Entity ${SOURCES})
target_link_libraries(Entity 
    Math
    Animation
    Util
    
    Debug
)
target_include_directories(Entity PUBLIC include 
    ${DEPS_FOLDER} 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${COMMON_DIR}
)