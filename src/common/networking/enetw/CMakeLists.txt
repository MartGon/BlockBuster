SET(SOURCES

    Address.cpp
    Exception.cpp
    Host.cpp
    HostFactory.cpp
    Packet.cpp
    Peer.cpp
    
)


add_library(EnetW ${SOURCES})
target_link_libraries(EnetW Util enet)
target_include_directories(EnetW PUBLIC include ${DEPS_FOLDER} ${DEPS_FOLDER}/enet/include ${CMAKE_CURRENT_SOURCE_DIR} ${COMMON_DIR})