#include <glad/glad.h>

#include <glm/gtc/matrix_transform.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <iostream>

#include <gl/Shader.h>
#include <gl/VertexArray.h>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

int main()
{
    if(SDL_Init(0))
    {
        std::cout << "SDL Init failed: " << SDL_GetError() << "\n";
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    auto window = SDL_CreateWindow("SDL Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    if(!window)
    {
        std::cout << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        return -1;
    }
    auto context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);

    if(!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::cout << "gladLoadGLLoader failed \n";
        return -1;
    }
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    std::filesystem::path shadersDir{SHADERS_DIR};
    std::filesystem::path vertex = shadersDir / "vertex.glsl";
    std::filesystem::path fragment = shadersDir / "fragment.glsl";
    GL::Shader shader{vertex, fragment};
    shader.Use();

    GL::VertexArray vao;
    vao.GenVBO(std::vector<float>{
        -0.5, -0.5, -0.5,
        0.5, -0.5, -0.5,
        0.5, 0.5, -0.5,
        -0.5, 0.5, -0.5,
        
        -0.5, -0.5, 0.5,
        0.5, -0.5, 0.5,
        0.5, 0.5, 0.5,
        -0.5, 0.5, 0.5,
    });
    vao.SetIndices({
                    // Front Face
                    0, 1, 2,
                    3, 0, 2,

                    // Back face
                    4, 5, 6,
                    7, 4, 6,

                    // Left Face
                    0, 4, 3,
                    7, 4, 3,

                    // Right face
                    1, 2, 5,
                    6, 2, 5,

                    // Up face
                    2, 3, 6,
                    7, 3, 6,

                    // Down face
                    0, 1, 4,
                    5, 1, 4
                    });
    vao.AttribPointer(0, 3, GL_FLOAT, false, 0);
    
    glEnable(GL_DEPTH_TEST);

    bool quit = false;
    while(!quit)
    {
        SDL_Event e;
        while(SDL_PollEvent(&e) != 0)
        {
            switch(e.type)
            {
            case  SDL_QUIT:
                quit = true;
                break;
            case SDL_KEYDOWN:
                if(e.key.keysym.sym == SDLK_ESCAPE)
                    quit = true;
                break;
            }
        }

        glm::mat4 model{1.0f};
        model = glm::translate(model, glm::vec3{0.0f, 0.0f, -3.0f});
        auto rotation = glm::rotate(glm::mat4{1.0f}, glm::radians((float)SDL_GetTicks() / 8.f), glm::vec3{1.0f, 1.0f, 0.0f});
        auto scale = glm::scale(glm::mat4{1.0f}, glm::vec3(1.0f));
        model = model * rotation * scale;
        glm::mat4 perspective = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
        model = perspective * model;
        shader.SetUniformMat4("transform", model);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDrawElements(GL_TRIANGLES, vao.GetIndicesCount(), GL_UNSIGNED_INT, 0);

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}