add_subdirectory(enetw)

SET(SOURCES
    Command.cpp
    Snapshot.cpp
    Networking.cpp
)

add_library(Networking ${SOURCES})
target_link_libraries(Networking 
    Util 
    EnetW
    Entity

    enet 
)
target_include_directories(Networking PUBLIC 
    include 
    ${DEPS_FOLDER} 
    ${DEPS_FOLDER}/enet/include 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${COMMON_DIR}
)