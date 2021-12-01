add_subdirectory(glad)
add_subdirectory(imgui)
add_subdirectory(mglogger)
add_subdirectory(enet)

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)