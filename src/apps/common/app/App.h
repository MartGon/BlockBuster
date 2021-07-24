#pragma once

#include <glad/glad.h>
#include <SDL2/SDL.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl.h>

#include <Configuration.h>

#include <exception>
#include <stdexcept>

namespace App
{
    class App
    {
    public:
        App(Configuration config);
        virtual ~App();

        virtual void Start() = 0;
        virtual void Update() = 0;
        virtual bool Quit() = 0;
    
    protected:
        Configuration config;

        ImGuiIO* io_;
        SDL_Window* window_;
        SDL_GLContext context_;
    };

    class InitError : public std::runtime_error
    {
    public:
        InitError(const char* msg) : runtime_error(msg) {}
    };
}