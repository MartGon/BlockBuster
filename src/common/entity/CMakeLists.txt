SET(SOURCES
    Block.cpp
    Map.cpp
    Game.cpp
    Player.cpp
)


add_library(BlockBuster ${SOURCES})
target_link_libraries(BlockBuster 
    Math
    
    Debug
)
target_include_directories(BlockBuster PUBLIC include 
    ${DEPS_FOLDER} 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${COMMON_DIR}
)