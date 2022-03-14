#pragma once

#include <glad/glad.h>
#include <SDL2/SDL.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <gui/GUI.h>

#include <Configuration.h>

#include <mglogger/MGLogger.h>

#include <exception>
#include <stdexcept>

namespace App
{
    class AppI
    {
    friend class VideoSettingsPopUp;
    public:
        AppI(Configuration config);
        virtual ~AppI();

        virtual void Start() {};
        virtual void Update() = 0;
        virtual bool Quit() = 0;
        virtual void Shutdown() {};
    
        Configuration config;
    protected:

        glm::ivec2 GetWindowSize();
        void SetWindowSize(glm::ivec2 size);
        glm::ivec2 GetMousePos();
        void RenameMainWindow(const std::string& name);
        
        virtual void ApplyVideoOptions(Configuration::WindowConfig& winConfig);

        std::string GetConfigOption(const std::string& key, std::string defaultValue);

        ImGuiIO* io_;
        SDL_Window* window_;
        SDL_GLContext context_;
        Log::Logger* logger;
    };

    class InitError : public std::runtime_error
    {
    public:
        InitError(const char* msg) : runtime_error(msg) {}
    };
}