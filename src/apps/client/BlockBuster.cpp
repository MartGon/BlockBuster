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

#include <Client.h>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

float FixFloat(float a, float precision)
{
    return (float)round(a / precision) * precision;
}

int main()
{
    App::Configuration config{
        App::Configuration::WindowConfig{
            "Client",
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
        },
        App::Configuration::OpenGLConfig{
            4, 6, SDL_GL_CONTEXT_PROFILE_CORE
        }
    };
    BlockBuster::Client client(config);
    client.Start();

    while(!client.Quit())
    {
        client.Update();
    }

    return 0;
}
