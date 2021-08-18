SET(SOURCES
    Test.cpp
)

add_executable(BlockBusterTests ${SOURCES})
target_include_directories(BlockBusterTests PUBLIC 
    ${CMAKE_SOURCE_DIR}/deps
    ${CMAKE_SOURCE_DIR}/src/common/
)
target_link_libraries(BlockBusterTests
    Game
    Math
    
    Debug
)

add_test(NAME RunTests COMMAND BlockBusterTests)