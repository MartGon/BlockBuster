#include <Client.h>

#include <iostream>
#include <algorithm>

void BlockBuster::Client::Start()
{
    glEnable(GL_MULTISAMPLE);

    // Shaders
    shader.Use();

    // Meshes
    circle = Rendering::Primitive::GenerateCircle(0.5f, 64);
}

void BlockBuster::Client::Update()
{
    HandleSDLEvents();

    // Clear Buffer
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw data
    glm::mat4 t{1.f};
    shader.SetUniformMat4("transform", t);
    shader.SetUniformVec4("color", glm::vec4{1.0f});
    circle.Draw(shader);

    SDL_GL_SwapWindow(window_);
}

bool BlockBuster::Client::Quit()
{
    return quit;
}

void BlockBuster::Client::HandleSDLEvents()
{
    SDL_Event e;
    
    while(SDL_PollEvent(&e) != 0)
    {
        switch(e.type)
        {
        case SDL_QUIT:
            quit = true;
            break;
        }
    }
}