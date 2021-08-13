SET(SOURCES
    Block.cpp
    Map.cpp
    Game.cpp
)


add_library(Game ${SOURCES})
target_link_libraries(Game 
    Math
    
    Debug
)
target_include_directories(Game PUBLIC include 
    ${DEPS_FOLDER} 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${COMMON_DIR}
)