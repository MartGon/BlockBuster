SET(SOURCES

    Client.cpp
    Main.cpp
)

add_executable(Client ${SOURCES})
target_link_libraries(Client 
    imgui 
    GLAD 
    ${X11_LIBRARIES} 
    ${OPENGL_LIBRARIES}
    ${SDL2_LIBRARIES}
    Threads::Threads 
    Collisions 
    GL 
    Rendering 
    Math 
    BlockBuster 
    App
    Game

    EnetW
    enet
    Networking
)
target_include_directories(Client PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OPENGL_INCLUDE_DIR} 
    ${DEPS_FOLDER} 
    ${DEPS_FOLDER}/imgui
    ${DEPS_FOLDER}/enet/include
    ${COMMON_DIR} 
    ${APPS_COMMON_DIR}
)