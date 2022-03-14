#include <VideoSettingsPopUp.h>

#include <SDL2/SDL.h>

using namespace App;

VideoSettingsPopUp::VideoSettingsPopUp(AppI& app) : app{&app}
{
    SetTitle("Video Settings"); // Bug: This glitches everything, for some reason
    SetFlags(ImGuiWindowFlags_NoResize);
}

static std::vector<SDL_DisplayMode> GetDisplayModes()
{
    std::vector<SDL_DisplayMode> displayModes;

    int displays = SDL_GetNumVideoDisplays();
    for(int i = 0; i < 1; i++)
    {
        auto numDisplayModes = SDL_GetNumDisplayModes(i);

        displayModes.reserve(numDisplayModes);
        for(int j = 0; j < numDisplayModes; j++)
        {
            SDL_DisplayMode mode;
            if(!SDL_GetDisplayMode(i, j, &mode))
            {
                displayModes.push_back(mode);
            }
        }
    }

    return displayModes;
}

static std::string DisplayModeToString(int w, int h, int rr)
{
    return std::to_string(w) + " x " + std::to_string(h) + " " + std::to_string(rr) + " Hz";
}

static std::string DisplayModeToString(SDL_DisplayMode mode)
{
    return std::to_string(mode.w) + " x " + std::to_string(mode.h) + " " + std::to_string(mode.refresh_rate) + " Hz";
}

void VideoSettingsPopUp::OnDraw()
{
    std::string resolution = DisplayModeToString(winConfig.resolutionW, winConfig.resolutionH, winConfig.refreshRate);
    if(ImGui::BeginCombo("Resolution", resolution.c_str()))
    {
        auto displayModes = GetDisplayModes();
        for(auto& mode : displayModes)
        {
            bool selected = winConfig.mode == mode.w && winConfig.mode == mode.h && winConfig.refreshRate == mode.refresh_rate;
            if(ImGui::Selectable(DisplayModeToString(mode).c_str(), selected))
            {
                winConfig.resolutionW = mode.w;
                winConfig.resolutionH = mode.h;
                winConfig.refreshRate = mode.refresh_rate;
            }
        }

        ImGui::EndCombo();
    }

    ImGui::Text("Window Mode");
    ImGui::RadioButton("Windowed", &winConfig.mode, ::App::Configuration::WindowMode::WINDOW); ImGui::SameLine();
    ImGui::RadioButton("Fullscreen", &winConfig.mode, ::App::Configuration::WindowMode::FULLSCREEN); ImGui::SameLine();
    ImGui::RadioButton("Borderless", &winConfig.mode, ::App::Configuration::WindowMode::BORDERLESS);

    ImGui::Checkbox("Vsync", &winConfig.vsync);

    int fov = glm::degrees(winConfig.fov);
    if(ImGui::SliderInt("FOV", &fov, 45, 90))
    {
        winConfig.fov = glm::radians((float)fov);
        std::cout << "Preconfig fov is " << winConfig.fov << "\n";
    }

    if(ImGui::Button("Accept"))
    {
        app->ApplyVideoOptions(winConfig);
        app->config.window = winConfig;
        Close();
    }

    ImGui::SameLine();
    if(ImGui::Button("Apply"))
    {
        app->ApplyVideoOptions(winConfig);
    }

    ImGui::SameLine();
    if(ImGui::Button("Cancel"))
    {   
        app->ApplyVideoOptions(oldWinConfig);
        Close();
    }
}

void VideoSettingsPopUp::OnOpen()
{
    winConfig = app->config.window;
    oldWinConfig = app->config.window;
}

void VideoSettingsPopUp::OnClose()
{
    app->ApplyVideoOptions(oldWinConfig);
    if(onClose)
        onClose();
}