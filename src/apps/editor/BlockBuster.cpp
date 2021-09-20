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

#include <game/Block.h>
#include <game/Player.h>

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
            4, 6, SDL_GL_CONTEXT_PROFILE_CORE, SHADERS_DIR
        }
    };

    std::filesystem::path configPath("editor.ini");
    try
    {
        config = App::LoadConfig(configPath);
    }
    catch (const std::out_of_range& e)
    {
        std::cerr << "Configuration file is corrupted:" << e.what() << '\n';
        std::cerr << "Either fix or remove it to generate the default one\n";
        std::exit(-1);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        std::cerr << "Loading default config\n";
    }

    
    try {
        BlockBuster::Editor::Editor editor(config);
        editor.Start();

        while (!editor.Quit())
        {
            editor.Update();
        }
        editor.Shutdown();

        App::WriteConfig(editor.config, configPath);

        std::cout << "Quitting\n";
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << e.what() << '\n';
        std::exit(-1);
    }

    return 0;
}
