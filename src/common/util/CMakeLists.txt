SET(SOURCES
    
    Buffer.cpp
    Ring.cpp
    Random.cpp
    Result.cpp
    Time.cpp
    Timer.cpp
    File.cpp
    Zip.cpp
    Table.cpp
)


add_library(Util ${SOURCES})
target_link_libraries(Util 
    Debug
    zip
)
target_include_directories(Util PUBLIC 
    include 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${COMMON_DIR}
    ${DEPS_FOLDER}
)