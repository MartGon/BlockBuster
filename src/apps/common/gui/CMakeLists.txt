SET(SOURCES
    PopUp.cpp
)


add_library(GUI ${SOURCES})
target_link_libraries(GUI 
    GLAD 
    Util
    imgui
    ${X11_LIBRARIES} 
    ${OPENGL_LIBRARIES} 
    ${CMAKE_DL_LIBS} 
    ${SDL2_LIBRARIES} 
    Threads::Threads
)
target_include_directories(GUI PUBLIC 
    
    ${DEPS_FOLDER}/imgui
    ${OPENGL_INCLUDE_DIR} 
    ${GLAD_INCLUDE_DIR} 
    ${DEPS_FOLDER} 
    ${CMAKE_CURRENT_SOURCE_DIR}
)