SET(SOURCES
    Result.cpp
)


add_library(General ${SOURCES})
target_link_libraries(General 
    Debug
)
target_include_directories(General PUBLIC include 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${COMMON_DIR}
)