SET(SOURCES
    Test.cpp
    Util.cpp
    Math.cpp
    Leak.cpp
)

add_executable(BlockBusterTests ${SOURCES})
target_include_directories(BlockBusterTests PUBLIC 
    ${CMAKE_SOURCE_DIR}/deps
    ${CMAKE_SOURCE_DIR}/src/common/
    ${CMAKE_SOURCE_DIR}/src/apps/common/
)
target_link_libraries(BlockBusterTests
    Entity
    Math
    Util
    Game
    
    Debug
)

add_test(NAME RunTests COMMAND BlockBusterTests)