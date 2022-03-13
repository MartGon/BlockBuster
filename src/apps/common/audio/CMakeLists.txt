SET(SOURCES
    Audio.cpp
)


add_library(Audio ${SOURCES})
target_link_libraries(Audio 
    OpenAL

    Util
    Log
)
target_include_directories(Audio PUBLIC include 
    ${DEPS_FOLDER} 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${COMMON_DIR}
    ${DEPS_FOLDER}/openal-soft/include
)