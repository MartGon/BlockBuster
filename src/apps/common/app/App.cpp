#include <App.h>

#include <string>

App::App::App(Configuration config) : config{config}
{
    if(SDL_Init(0))
    {
        std::string msg = "SDL Init failed: " + std::string(SDL_GetError()) + "\n";
        throw InitError(msg.c_str());
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, config.openGL.majorVersion);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, config.openGL.minorVersion);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, config.openGL.profileMask);

    window_ = SDL_CreateWindow(config.window.name.c_str(), config.window.xPos, config.window.yPos, config.window.width, config.window.height, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    if(!window_)
    {
        std::string msg = "SDL_CreateWindow failed: " + std::string(SDL_GetError()) + "\n";
        throw InitError(msg.c_str());
    }
    context_ = SDL_GL_CreateContext(window_);
    SDL_GL_MakeCurrent(window_, context_);

    if(!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::string msg =  "gladLoadGLLoader failed \n";
        throw InitError(msg.c_str());
    }
    glViewport(0, 0, config.window.width, config.window.height);

    // ImGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(window_, context_);
    ImGui_ImplOpenGL3_Init("#version 330");
}

App::App::~App()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(context_);
    SDL_DestroyWindow(window_);

    SDL_Quit();
}