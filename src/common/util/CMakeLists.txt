SET(SOURCES
    Result.cpp
    Buffer.cpp
    Random.cpp
    Time.cpp
)


add_library(Util ${SOURCES})
target_link_libraries(Util 
    Debug
)
target_include_directories(Util PUBLIC include 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${COMMON_DIR}
)