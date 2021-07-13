SET(SOURCES

    Transform.cpp
)


add_library(Math ${SOURCES})
target_link_libraries(Math)
target_include_directories(Math PUBLIC include ${DEPS_FOLDER} ${CMAKE_CURRENT_SOURCE_DIR})