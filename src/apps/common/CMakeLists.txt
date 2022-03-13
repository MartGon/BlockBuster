SET(APPS_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
SET(APPS_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(gl)
add_subdirectory(rendering)
add_subdirectory(game)
add_subdirectory(app)
add_subdirectory(gui)
add_subdirectory(audio)