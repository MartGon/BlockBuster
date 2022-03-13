SET(SOURCES
    Audio.cpp
)


add_library(Audio ${SOURCES})
target_link_libraries(Audio 
    Log
)
target_include_directories(Audio PUBLIC include 
    ${DEPS_FOLDER} 
    ${CMAKE_CURRENT_SOURCE_DIR}
)