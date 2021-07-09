SET(SOURCES

    Collisions.cpp
)


add_library(Collisions ${SOURCES})
target_link_libraries(Collisions )
target_include_directories(Collisions PUBLIC include ${DEPS_FOLDER} ${CMAKE_CURRENT_SOURCE_DIR})