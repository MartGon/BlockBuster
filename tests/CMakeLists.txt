SET(SOURCES
    Test.cpp
    Util.cpp
)

add_executable(BlockBusterTests ${SOURCES})
target_include_directories(BlockBusterTests PUBLIC 
    ${CMAKE_SOURCE_DIR}/deps
    ${CMAKE_SOURCE_DIR}/src/common/
)
target_link_libraries(BlockBusterTests
    Entity
    Math
    Util
    
    Debug
)

add_test(NAME RunTests COMMAND BlockBusterTests)