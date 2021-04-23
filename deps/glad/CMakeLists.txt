
add_library(GLAD src/glad.c)

set(GLAD_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE INTERNAL "")

target_include_directories(GLAD PUBLIC ${GLAD_INCLUDE_DIR})