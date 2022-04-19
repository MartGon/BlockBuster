
if(BUILD_APPS)
    add_subdirectory(glad)
    add_subdirectory(imgui)
    add_subdirectory(freetype)
    add_subdirectory(zip)
    add_subdirectory(base64)

    set(LIBTYPE "STATIC")
    add_subdirectory(openal-soft)
endif()

add_subdirectory(enet)
add_subdirectory(mglogger)

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)