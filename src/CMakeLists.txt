add_subdirectory(common)
add_subdirectory(server)

if(BUILD_APPS)
    add_subdirectory(apps)
endif()