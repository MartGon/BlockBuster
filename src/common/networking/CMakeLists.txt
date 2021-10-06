SET(SOURCES

    Exception.cpp
    Host.cpp
    HostFactory.cpp
    Packet.cpp
    Peer.cpp
    
)


add_library(Networking ${SOURCES})
target_link_libraries(Networking Util enet)
target_include_directories(Networking PUBLIC include ${DEPS_FOLDER} ${DEPS_FOLDER}/enet/include ${CMAKE_CURRENT_SOURCE_DIR} ${COMMON_DIR})