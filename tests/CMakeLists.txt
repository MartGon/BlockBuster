SET(SOURCES
    Test.cpp
)

add_executable(BlockBusterTests ${SOURCES})
target_include_directories(BlockBusterTests PUBLIC ${CMAKE_SOURCE_DIR}/deps)

add_test(NAME RunTests COMMAND BlockBusterTests)