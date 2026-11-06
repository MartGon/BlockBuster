
if(BUILD_APPS)
    add_subdirectory(glad)
    add_subdirectory(imgui)
    add_subdirectory(freetype)
    add_subdirectory(base64)

    set(LIBTYPE "STATIC")

    # OpenAL tries OSS before SDL2, and OSS doesn't fail until the device is opened, so it has to be disabled
    if(USE_SDL2_AUDIO)
        set(ALSOFT_BACKEND_SDL2 ON CACHE BOOL "Enable SDL2 backend" FORCE)
        set(ALSOFT_BACKEND_OSS OFF CACHE BOOL "Enable OSS backend" FORCE)
    else()
        set(ALSOFT_BACKEND_SDL2 OFF CACHE BOOL "Enable SDL2 backend" FORCE)
        set(ALSOFT_BACKEND_OSS ON CACHE BOOL "Enable OSS backend" FORCE)
    endif()
    add_subdirectory(openal-soft)
endif()

add_subdirectory(enet)
add_subdirectory(mglogger)
add_subdirectory(zip)

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)