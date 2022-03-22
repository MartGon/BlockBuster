
set(SOURCES
    base64.cpp
)

add_library(base64 ${SOURCES})
target_include_directories(base64 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )
