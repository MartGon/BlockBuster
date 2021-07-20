SET(SOURCES

    Player.cpp
)


add_library(AppsGame ${SOURCES})
target_link_libraries(AppsGame 
    GLAD 
    ${X11_LIBRARIES} 
    ${OPENGL_LIBRARIES} 
    ${CMAKE_DL_LIBS} 
    ${SDL2_LIBRARIES} 
    Threads::Threads 
    GL 
    Rendering
    Collisions 
    Math
)
target_include_directories(AppsGame PUBLIC include 
    ${OPENGL_INCLUDE_DIR}  
    ${GLAD_INCLUDE_DIR} 
    ${DEPS_FOLDER} 
    ${CMAKE_CURRENT_SOURCE_DIR}
)