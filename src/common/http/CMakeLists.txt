SET(SOURCES
    AsyncClient.cpp
)

add_library(HTTP ${SOURCES})
target_link_libraries(HTTP 
    Util 
)
target_include_directories(HTTP PUBLIC 
    include 
    ${DEPS_FOLDER} 
    ${DEPS_FOLDER}/enet/include 
    ${CMAKE_CURRENT_SOURCE_DIR} 
)