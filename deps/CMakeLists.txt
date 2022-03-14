add_subdirectory(glad)
add_subdirectory(imgui)
add_subdirectory(mglogger)
add_subdirectory(enet)
add_subdirectory(freetype)

set(LIBTYPE "STATIC")
add_subdirectory(openal-soft)

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)