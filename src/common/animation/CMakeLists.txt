SET(SOURCES

    Animation.cpp
)


add_library(Animation ${SOURCES})
target_link_libraries(Animation 
    Math
)
target_include_directories(Animation PUBLIC 
    include 
    ${DEPS_FOLDER} 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${COMMON_DIR}
)