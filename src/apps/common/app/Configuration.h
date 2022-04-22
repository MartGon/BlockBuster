#pragma once 
#include <SDL2/SDL.h>

#include <glm/glm.hpp>
#include <mglogger/MGLogger.h>

#include <string>
#include <filesystem>
#include <unordered_map>

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

        struct WindowConfig
        {
            std::string name;
            int resolutionW;
            int resolutionH;
            int xPos;
            int yPos;
            WindowMode mode;
            bool vsync = true;
            int refreshRate;
            float fov;
            int renderDistance;
        };

        struct OpenGLConfig
        {
            int majorVersion;
            int minorVersion;
            int profileMask;
            bool antialiasing;
            int msaaSamples;
            std::filesystem::path shadersFolder;
        };

        struct LogConfig
        {
            std::filesystem::path logFile;
            Log::Verbosity verbosity;
        };

        WindowConfig window;
        OpenGLConfig openGL;
        LogConfig log;

        // Extra options by application
        std::unordered_map<std::string, std::string> options;
    };

    void WriteConfig(Configuration config, std::filesystem::path filePath);
    Configuration LoadConfig(std::filesystem::path filepath);
}