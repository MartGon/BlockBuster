#include <App.h>

#include <game/ServiceLocator.h>

#include <string>

#include <iostream>

using namespace App;

#ifdef _DEBUG

void GLAPIENTRY ErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* msg, const void* userParam)
{
    if(severity != GL_DEBUG_SEVERITY_NOTIFICATION)
    {
        std::cout << "GL Event\n";
        std::cout << "Type: " << (type == GL_DEBUG_TYPE_ERROR ? "Error" : "Event") << "\n";
        std::cout << "Type code: " << std::hex << type << "\n";
        std::cout << "Id: " << std::hex << id << "\n";
        std::cout << "Severity: " << std::hex << severity << "\n";
        std::cout << "Message: " << msg << "\n";
        std::cout << "\n";
    }
}

#endif

AppI::AppI(Configuration config) : config{config}, logger{ServiceLocator::GetLogger()}
{
    // SDL
    if(SDL_Init(0))
    {
        std::string msg = "SDL Init failed: " + std::string(SDL_GetError()) + "\n";
        logger->LogCritical(msg);
        throw InitError(msg.c_str());
    }

    if(SDL_InitSubSystem(SDL_INIT_VIDEO))
    {
        std::string msg = "SDL Init Video failed: " + std::string(SDL_GetError()) + "\n";
        logger->LogCritical(msg);
        throw InitError(msg.c_str());
    }

    if(config.openGL.antialiasing)
    {
        logger->LogDebug("AA enabled");
	    //SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, config.openGL.msaaSamples);
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, config.openGL.majorVersion);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, config.openGL.minorVersion);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, config.openGL.profileMask);

    window_ = SDL_CreateWindow(config.window.name.c_str(), config.window.xPos, config.window.yPos, config.window.resolutionW, config.window.resolutionH, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if(!window_)
    {
        std::string msg = "SDL_CreateWindow failed: " + std::string(SDL_GetError()) + "\n";
        logger->LogCritical(msg);
        throw InitError(msg.c_str());
    }
    SDL_SetWindowFullscreen(window_, config.window.mode);

    context_ = SDL_GL_CreateContext(window_);
    SDL_GL_MakeCurrent(window_, context_);

    if(!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::string msg =  "gladLoadGLLoader failed \n";
        logger->LogCritical(msg);
        throw InitError(msg.c_str());
    }

    #ifdef _DEBUG
        glEnable(GL_DEBUG_OUTPUT);
        //glDebugMessageCallback(ErrorCallback, 0); -- Only usable in OpenGL ^4.3
    #endif

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

AppI::~AppI()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(context_);
    SDL_DestroyWindow(window_);

    logger->LogDebug("Quitting SDL_INIT_VIDEO");
    SDL_QuitSubSystem(SDL_INIT_VIDEO);

    logger->LogDebug("Quitting SDL");
    SDL_Quit();
}

// Protected

glm::ivec2 AppI::GetWindowSize()
{
    glm::ivec2 size;
    SDL_GetWindowSize(window_, &size.x, &size.y);

    return size;
}

void AppI::SetWindowSize(glm::ivec2 size)
{
    auto width = size.x; auto height = size.y;
    SDL_SetWindowSize(window_, width, height);
    SDL_GetWindowSize(window_, &width, &height);
    glViewport(0, 0, width, height);
}

glm::ivec2 AppI::GetMousePos()
{
    glm::ivec2 mousePos;
    SDL_GetMouseState(&mousePos.x, &mousePos.y);
    mousePos.y = GetWindowSize().y - mousePos.y;

    return mousePos;
}


void AppI::SetMouseGrab(bool grab)
{
    SDL_SetWindowGrab(this->window_, (SDL_bool)grab);
    SDL_SetRelativeMouseMode((SDL_bool)grab);
}


void AppI::RenameMainWindow(const std::string& name)
{
    SDL_SetWindowTitle(window_, name.c_str());
}

Log::Logger* AppI::GetLogger()
{
    return logger;
}

void AppI::ApplyVideoOptions(::App::Configuration::WindowConfig& winConfig)
{
    auto width = winConfig.resolutionW;
    auto height = winConfig.resolutionH;
    SDL_SetWindowFullscreen(window_, winConfig.mode);

    if(winConfig.mode == ::App::Configuration::FULLSCREEN)
    {
        SDL_SetWindowDisplayMode(window_, NULL);
        auto display = SDL_GetWindowDisplayIndex(window_);
        SDL_DisplayMode mode;
        SDL_GetDesktopDisplayMode(display, &mode);
        width = mode.w;
        height = mode.h;
    }
    SetWindowSize(glm::ivec2{width, height});
    
    SDL_GL_SetSwapInterval(winConfig.vsync);

    SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

std::string AppI::GetConfigOption(const std::string& key, std::string defaultValue)
{
    std::string ret = defaultValue;
    auto it = config.options.find(key);

    if(it != config.options.end())
    {
        ret = it->second;
    }

    return ret;
}
