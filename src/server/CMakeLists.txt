SET(SOURCES

    Main.cpp
    Server.cpp
    ServerEvent.cpp
)

add_executable(Server ${SOURCES})
target_link_libraries(Server

    Collisions 
    Math
    Log
    Networking
    EnetW
    enet

    HTTP
    nlohmann_json::nlohmann_json
)

target_include_directories(Server PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${DEPS_FOLDER} 
    ${DEPS_FOLDER}/enet/include
    ${DEPS_FOLDER}/json/include
    ${COMMON_DIR}
)