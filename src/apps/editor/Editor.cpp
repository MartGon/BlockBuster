#include <Editor.h>

#include <iostream>

#include <glm/gtc/constants.hpp>

void BlockBuster::Editor::Start()
{
    // Shaders
    shader.Use();

    // Meshes
    cube = Rendering::Primitive::GenerateCube();
    slope = Rendering::Primitive::GenerateSlope();

    // Textures
    texture = GL::Texture::FromFolder(TEXTURES_DIR, "SmoothStone.png");
    try
    {
        texture.Load();
    }
    catch(const GL::Texture::LoadError& e)
    {
        std::cout << "Error when loading texture " + e.path_.string() + ": " +  e.what() << '\n';
    }
    
    // OpenGL features
    glEnable(GL_DEPTH_TEST);

    // Camera pos
    camera.SetPos(glm::vec3 {0.0f, 6.0f, 6.0f});
    camera.SetParam(Rendering::Camera::Param::ASPECT_RATIO, (float)config.window.width / (float)config.window.height);
    camera.SetTarget(glm::vec3{0.0f});

    // World
    blocks = {
        {Math::Transform{glm::vec3{0.0f, 0.0f, 0.0f} * BLOCK_SCALE, glm::vec3{0.0f, 0.0f, 0.0f}, BLOCK_SCALE}, Game::BLOCK}
    };
}

void BlockBuster::Editor::Update()
{
    bool clicked = false;
    glm::vec2 mousePos;

    SDL_Event e;
    while(SDL_PollEvent(&e) != 0)
    {
        ImGui_ImplSDL2_ProcessEvent(&e);
        switch(e.type)
        {
        case SDL_KEYDOWN:
            if(e.key.keysym.sym == SDLK_ESCAPE)
                quit = true;
            break;
        case SDL_QUIT:
            std::cout << "Quitting\n";
            quit = true;
            break;
        case SDL_MOUSEBUTTONDOWN:
            mousePos.x = e.button.x;
            mousePos.y = config.window.height - e.button.y;
            clicked = true;
            std::cout << "Click coords " << mousePos.x << " " << mousePos.y << "\n";
        }
    }

    UpdateCamera();

    // Draw cubes/slopes
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for(int i = 0; i < blocks.size(); i++)
    {
        auto model = blocks[i].transform.GetTransformMat();
        auto type = blocks[i].type;
        auto transform = camera.GetProjViewMat() * model;

        shader.SetUniformInt("isPlayer", 0);
        shader.SetUniformMat4("transform", transform);
        if(type == Game::SLOPE)
        {
            slope.Draw(shader, &texture);
        }
        else
        {
            cube.Draw(shader, &texture);
        }
    }

    SDL_GL_SwapWindow(window_);
}

bool BlockBuster::Editor::Quit()
{
    return quit;
}

void BlockBuster::Editor::UpdateCamera()
{
    auto state = SDL_GetKeyboardState(nullptr);

    auto cameraRot = camera.GetRotation();
    float pitch = 0.0f;
    float yaw = 0.0f;
    if(state[SDL_SCANCODE_UP])
        pitch += -CAMERA_ROT_SPEED;
    if(state[SDL_SCANCODE_DOWN])
        pitch += CAMERA_ROT_SPEED;

    if(state[SDL_SCANCODE_LEFT])
        yaw += CAMERA_ROT_SPEED;
    if(state[SDL_SCANCODE_RIGHT])
        yaw += -CAMERA_ROT_SPEED;
    
    cameraRot.x = glm::min(cameraRot.x + pitch, glm::pi<float>() - 0.01f);
    cameraRot.y = glm::min(cameraRot.y + yaw, glm::two_pi<float>());
    camera.SetRotation(cameraRot.x, cameraRot.y);
    
    auto cameraPos = camera.GetPos();
    auto front = camera.GetFront();
    auto xAxis = glm::normalize(-glm::cross(front, Rendering::Camera::UP));
    auto zAxis = glm::normalize(-glm::cross(xAxis, Rendering::Camera::UP));
    glm::vec3 moveDir{0};
    if(state[SDL_SCANCODE_A])
        moveDir += xAxis;
    if(state[SDL_SCANCODE_D])
        moveDir -= xAxis;
    if(state[SDL_SCANCODE_W])
        moveDir -= zAxis;
    if(state[SDL_SCANCODE_S])
        moveDir += zAxis;
    if(state[SDL_SCANCODE_Q])
        moveDir.y += 1;
    if(state[SDL_SCANCODE_E])
        moveDir.y -= 1;
    if(state[SDL_SCANCODE_F])
        cameraPos = glm::vec3{0.0f, 2.0f, 0.0f};
           
    auto offset = glm::length(moveDir) > 0.0f ? (glm::normalize(moveDir) * CAMERA_MOVE_SPEED) : moveDir;
    cameraPos += offset;
    camera.SetPos(cameraPos);
}