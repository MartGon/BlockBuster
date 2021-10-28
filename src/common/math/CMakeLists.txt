SET(SOURCES

    Interpolation.cpp
    Transform.cpp
    Math.cpp
)


add_library(Math ${SOURCES})
target_link_libraries(Math Debug)
target_include_directories(Math PUBLIC include ${DEPS_FOLDER} ${CMAKE_CURRENT_SOURCE_DIR} ${COMMON_DIR})