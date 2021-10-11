#include <glad/glad.h>

#include <glm/gtc/matrix_transform.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <iostream>
#include <algorithm>

#include <gl/Shader.h>
#include <gl/VertexArray.h>
#include <gl/Texture.h>

#include <rendering/Camera.h>
#include <rendering/Mesh.h>
#include <rendering/Primitive.h>
#include <rendering/Rendering.h>

#include <collisions/Collisions.h>

#include <math/Transform.h>

#include <entity/Block.h>
#include <game/ServiceLocator.h>
#include <game/PlayerController.h>

#include <Editor.h>

const int WINDOW_WIDTH = 1024;
const int WINDOW_HEIGHT = 720;

int main(int argc, char* args[])
{
    App::Configuration config{
        App::Configuration::WindowConfig{
            "Editor",
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            App::Configuration::WindowMode::WINDOW,
            true,
            60,
            glm::radians(60.0f)
        },
        App::Configuration::OpenGLConfig{
            4, 6, SDL_GL_CONTEXT_PROFILE_CORE, 1, 8, SHADERS_DIR
        },
        App::Configuration::LogConfig{
            "./editor.log", Log::Verbosity::ERROR
        }
    };

    // Init logger
    auto cLogger = std::make_unique<Log::ComposedLogger>();
    auto consoleLogger = std::make_unique<Log::ConsoleLogger>();
    cLogger->AddLogger(std::move(consoleLogger));

    auto filelogger = std::make_unique<Log::FileLogger>();
    filelogger->OpenLogFile(config.log.logFile);
    
    if(filelogger->IsOk())
        cLogger->AddLogger(std::move(filelogger));
    else
    {
        std::string msg = "Could not open log file: " + std::string(config.log.logFile) + '\n';
        cLogger->LogError(msg);
    }

    // Load config
    std::filesystem::path configPath("editor.ini");
    try
    {
        config = App::LoadConfig(configPath);
    }
    catch (const std::out_of_range& e)
    {
        App::ServiceLocator::GetLogger()->LogError(std::string("Configuration file is corrupted:") + e.what() + '\n');
        App::ServiceLocator::GetLogger()->LogError("Either fix or remove it to generate the default one\n");
        std::exit(-1);
    }
    catch (const std::exception& e)
    {
        App::ServiceLocator::GetLogger()->LogError(std::string(e.what()) + '\n');
        App::ServiceLocator::GetLogger()->LogInfo("Loading default config\n");
    }

    // Set logger
    cLogger->SetVerbosity(config.log.verbosity);
    App::ServiceLocator::SetLogger(std::move(cLogger));

    try {
        BlockBuster::Editor::Editor editor(config);
        editor.Start();

        while(!editor.Quit())
        {
            editor.Update();
        }
        editor.Shutdown();
        config = editor.config;
    }
    catch (const std::runtime_error& e)
    {
        App::ServiceLocator::GetLogger()->LogError(std::string(e.what()) + '\n');
    }

    App::WriteConfig(config, configPath);

    return 0;
}
