SET(SOURCES

    Server.cpp
    Main.cpp
)

add_executable(Server ${SOURCES})
target_link_libraries(Server

    Collisions 
    Math
    Log
    Networking
    EnetW

    enet
)

target_include_directories(Server PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${DEPS_FOLDER} 
    ${DEPS_FOLDER}/enet/include
    ${COMMON_DIR}
)