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

    window_ = SDL_CreateWindow(config.window.name.c_str(), config.window.xPos, config.window.yPos, config.window.resolutionW, config.window.resolutionH, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if(!window_)
    {
        std::string msg = "SDL_CreateWindow failed: " + std::string(SDL_GetError()) + "\n";
        throw InitError(msg.c_str());
    }
    SDL_SetWindowFullscreen(window_, config.window.mode);

    context_ = SDL_GL_CreateContext(window_);
    SDL_GL_MakeCurrent(window_, context_);

    if(!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::string msg =  "gladLoadGLLoader failed \n";
        throw InitError(msg.c_str());
    }

    // Get window size after enabling fullscreen
    int width, height;
    SDL_GetWindowSize(window_, &width, &height);
    if(config.window.mode == Configuration::FULLSCREEN)
    {
        SDL_SetWindowDisplayMode(window_, NULL);
        auto display = SDL_GetWindowDisplayIndex(window_);
        SDL_DisplayMode mode;
        SDL_GetDesktopDisplayMode(display, &mode);
        width = mode.w;
        height = mode.h;

        SDL_SetWindowSize(window_, width, height);
    }
    SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_GL_SetSwapInterval(config.window.vsync);

    glViewport(0, 0, width, height);

    // ImGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io_ = &ImGui::GetIO();

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

// Protected

glm::vec<2, int> App::App::GetWindowSize()
{
    glm::vec<2, int> size;
    SDL_GetWindowSize(window_, &size.x, &size.y);

    return size;
}

glm::vec<2, int> App::App::GetMousePos()
{
    glm::vec<2, int> mousePos;
    SDL_GetMouseState(&mousePos.x, &mousePos.y);
    mousePos.y = GetWindowSize().y - mousePos.y;

    return mousePos;
}

void App::App::RenameMainWindow(const std::string& name)
{
    std::string title = "Editor - " + name;
    SDL_SetWindowTitle(window_, title.c_str());
}