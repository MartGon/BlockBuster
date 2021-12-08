#include <GameState/MainMenu.h>

#include <Client.h>

#include <httplib/httplib.h>
#include <nlohmann/json.hpp>

#include <imgui/imgui_internal.h>

using namespace BlockBuster;

MainMenu::MainMenu(Client* client)  : GameState{client}
{

}

MainMenu::~MainMenu()
{
    if(reqThread.joinable())
        reqThread.join();
}

void MainMenu::Start()
{
    username[0] = '\0';
}

void MainMenu::Update()
{
    HandleSDLEvents();
    Render();
}

void MainMenu::Login()
{
    this->client_->logger->LogDebug("Login with rest service");

    connecting = true;

    if(reqThread.joinable())
        reqThread.join();

    reqThread = std::thread{
        [this](){
            httplib::Client client{"127.0.0.1", 3030};

            nlohmann::json body{{"username", this->username}};
            std::string bodyStr = nlohmann::to_string(body);

            auto res = client.Post("/login", bodyStr.c_str(), bodyStr.size(), "application/json");
            if(res)
            {
                auto value = res.value();
                auto body = value.body;
                client_->logger->LogInfo("Response: " + body);
            }
            else
            {
                auto errorCode = res.error();
                client_->logger->LogError("Could not connet to match-making server. Error Code: ");
            }

            client_->logger->Flush();

            Util::Time::Sleep(Util::Time::Seconds{3});

            this->mutex.lock();
            this->connecting = false;
            this->mutex.unlock();
        }
    };
}

void MainMenu::HandleSDLEvents()
{
    SDL_Event e;
    
    while(SDL_PollEvent(&e) != 0)
    {
        ImGui_ImplSDL2_ProcessEvent(&e);

        switch(e.type)
        {
        case SDL_QUIT:
            client_->quit = true;
            break;
        case SDL_KEYDOWN:
            break;
        }
    }
}

void MainMenu::Render()
{
    // Clear Buffer
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DrawGUI();

    // Swap buffers
    SDL_GL_SwapWindow(client_->window_);
}

void MainMenu::DrawGUI()
{
    // Clear GUI buffer
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(client_->window_);
    ImGui::NewFrame();

    auto displaySize = client_->io_->DisplaySize;
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    if(ImGui::Begin("Block Buster", nullptr, flags))
    {
        mutex.lock();
        auto itFlags = connecting ? ImGuiInputTextFlags_::ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;
        ImGui::InputText("Username", username, 16, itFlags);

        auto winWidth = ImGui::GetWindowWidth();
        auto buttonWidth = ImGui::CalcTextSize("Login").x + 8;
        ImGui::SetCursorPosX(winWidth / 2.0f - buttonWidth / 2.0f);

        auto enabled = connecting;
        if(enabled)
            ImGui::PushDisabled();

        if(ImGui::Button("Login"))
        {
            Login();
        }

        if(enabled)
            ImGui::PopDisabled();
        
        if(connecting)
            ImGui::Text("%s", "Connecting...");
        mutex.unlock();
    }
    ImGui::End();
    
    // Draw GUI
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData(), guiVao.GetHandle());
    auto windowSize = client_->GetWindowSize();
    int scissor_box[4] = { 0, 0, windowSize.x, windowSize.y };
    ImGui_ImplOpenGL3_RestoreState(scissor_box);
}