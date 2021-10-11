SET(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
SET(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(debug)
add_subdirectory(util)
add_subdirectory(math)
add_subdirectory(networking)
add_subdirectory(collisions)
add_subdirectory(entity)

#add_subdirectory(gl)