#include <SDL2/SDL.h>

#include <string>

namespace App
{
    struct Configuration
    {
        enum WindowMode
        {
            WINDOW,
            FULLSCREEN,
            BORDERLESS = SDL_WINDOW_FULLSCREEN_DESKTOP
        };

        struct  WindowConfig
        {
            std::string name;
            int width;
            int height;
            int xPos;
            int yPos;
            WindowMode mode;
            bool vsync = false;
        };

        struct OpenGLConfig
        {
            int majorVersion;
            int minorVersion;
            int profileMask;
        };

        WindowConfig window;
        OpenGLConfig openGL;
    };
}