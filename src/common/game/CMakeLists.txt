SET(SOURCES

    Block.cpp
)


add_library(Game ${SOURCES})
target_link_libraries(Game 
    Math
)
target_include_directories(Game PUBLIC include 
    ${DEPS_FOLDER} 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${COMMON_DIR}
)