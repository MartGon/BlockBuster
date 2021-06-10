
set(SOURCES
    imgui.cpp
    imgui_draw.cpp
    imgui_demo.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
    backends/imgui_impl_opengl3.cpp
    backends/imgui_impl_sdl.cpp
)

add_library(imgui ${SOURCES})
target_link_libraries(imgui GLAD ${X11_LIBRARIES} ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES})
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${OPENGL_INCLUDE_DIR} ${DEPS_FOLDER} ${SDL2_INCLUDE_DIRS})