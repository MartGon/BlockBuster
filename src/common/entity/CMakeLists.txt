SET(SOURCES
    Block.cpp
    Map.cpp
    Game.cpp
    Player.cpp
    Weapon.cpp
)


add_library(Entity ${SOURCES})
target_link_libraries(Entity 
    Math
    
    Debug
)
target_include_directories(Entity PUBLIC include 
    ${DEPS_FOLDER} 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${COMMON_DIR}
)