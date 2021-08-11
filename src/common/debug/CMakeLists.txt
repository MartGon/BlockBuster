SET(SOURCES
    Debug.cpp
)


add_library(Debug ${SOURCES})
target_link_libraries(Debug 
)
target_include_directories(Debug PUBLIC include 
    ${DEPS_FOLDER} 
    ${CMAKE_CURRENT_SOURCE_DIR} 
)