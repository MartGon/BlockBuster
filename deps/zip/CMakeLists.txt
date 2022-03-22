
set(SOURCES
    zip.c
)

add_library(zip ${SOURCES})
target_include_directories(zip PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )