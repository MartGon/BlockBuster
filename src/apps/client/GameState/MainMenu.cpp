#include <GameState/MainMenu.h>

#include <Client.h>

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
    inputUsername[0] = '\0';

    // Set default name
    popUp.SetTitle("Connecting");
    popUp.SetFlags(ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);
}

void MainMenu::Update()
{
    HandleRestResponses();
    HandleSDLEvents();
    Render();
}

void MainMenu::HandleRestResponses()
{
    while(auto mmRes = PollRestResponse())
    {
        auto mmResponse = std::move(mmRes.value());
        auto httpRes = std::move(mmResponse.httpRes);
        if(httpRes)
            mmResponse.onSuccess(httpRes.value());
        else
            mmResponse.onError(httpRes.error());
    }
}

std::optional<MainMenu::MMResponse> MainMenu::PollRestResponse()
{
    this->reqMutex.lock();
    auto ret = this->responses.PopFront();
    this->reqMutex.unlock();

    return ret;
}

void MainMenu::Login()
{
    GetLogger()->LogDebug("Login with rest service");

    // Raise pop up
    popUp.SetTitle("Login connection");
    popUp.SetText("Connnecting to match making server");
    popUp.SetVisible(true);
    popUp.SetButtonVisible(false);

    nlohmann::json body{{"username", this->inputUsername}};
    auto onSuccess = [this](httplib::Response& res){
        auto bodyStr = res.body;
        nlohmann::json body = nlohmann::json::parse(bodyStr);

        popUp.SetTitle("Login succesful");
        popUp.SetText("Your username is " + std::string(body["username"]));
        popUp.SetButtonVisible(true);
        popUp.SetButtonCallback([this](){
            GetLogger()->LogInfo("Button pressed. Opening server browser");
        });

        GetLogger()->LogInfo("Response: " + bodyStr);
        GetLogger()->Flush();
    };

    auto onError = [this](httplib::Error error){
        popUp.SetText("Could not connect to server");
        popUp.SetButtonVisible(true);

        GetLogger()->LogError("Could not connet to match-making server. Error Code: ");
    };
    Request("/login", body, onSuccess, onError);
}

void MainMenu::ListGames()
{
    GetLogger()->LogInfo("Requesting list of games");

    nlohmann::json body;
    auto onSuccess = [this](httplib::Response& res)
    {
        GetLogger()->LogInfo("List of games updated successfully");

        auto bodyStr = res.body;
        auto body = nlohmann::json::parse(bodyStr);

        auto games = body.at("games");
        this->gamesList.clear();
        for(auto game : games)
            this->gamesList.push_back(game);
    };

    auto onError = [this](httplib::Error error){
        popUp.SetVisible(true);
        popUp.SetText("Connection error");
        popUp.SetTitle("Error");
        popUp.SetButtonVisible(true);
        popUp.SetButtonCallback([this](){
            popUp.SetVisible(false);
        });
    };

    Request("/list_games", body, onSuccess, onError);
}

void MainMenu::JoinGame(std::string gameId)
{
    GetLogger()->LogInfo("Joining game " + gameId);

    // Show connecting pop up
    popUp.SetVisible(true);
    popUp.SetCloseable(true);
    popUp.SetTitle("Connecting");
    popUp.SetText("Joining game...");
    popUp.SetButtonVisible(false);

    nlohmann::json body;
    body["player_id"] = userId;
    body["game_id"] = gameId;

    auto onSuccess = [this, gameId](httplib::Response& res)
    {
        GetLogger()->LogInfo("Succesfullly joined game " + gameId);

        // Open game info window
        popUp.SetVisible(false);
    };

    auto onError = [this](httplib::Error err)
    {
        popUp.SetVisible(true);
        popUp.SetText("Connection error");
        popUp.SetTitle("Error");
        popUp.SetButtonVisible(true);
        popUp.SetButtonCallback([this](){
            popUp.SetVisible(false);
        });
    };

    Request("/join_game", body, onSuccess, onError);
}

void MainMenu::CreateGame(std::string name)
{
    nlohmann::json body;
    body["name"] = name;

    auto onSuccess = [this, name](httplib::Response& res)
    {
        /*
        auto bodyStr = res.body;
        auto body = nlohmann::json::parse(bodyStr);
        auto gameId = std::string(body["id"]);
        */
        GetLogger()->LogInfo("Succesfullly created game: " + name);

        // Open game info window -- Like joining a game
    };

    auto onError = [this](httplib::Error err)
    {
        popUp.SetVisible(true);
        popUp.SetText("Connection error");
        popUp.SetTitle("Error");
        popUp.SetButtonVisible(true);
        popUp.SetButtonCallback([this](){
            popUp.SetVisible(false);
        });
    };

    Request("/create_game", body, onSuccess, onError);
}

void MainMenu::Request(std::string endpoint, nlohmann::json body, 
    std::function<void(httplib::Response&)> onSuccess,
    std::function<void(httplib::Error)> onError)
{
    connecting = true;

    if(reqThread.joinable())
        reqThread.join();

    reqThread = std::thread{
        [this, body, endpoint, onSuccess, onError](){
            httplib::Client client{"127.0.0.1", 3030};

            std::string bodyStr = nlohmann::to_string(body);
            auto res = client.Post(endpoint.c_str(), bodyStr.c_str(), bodyStr.size(), "application/json");
            auto response = MMResponse{std::move(res), onSuccess, onError};
            Util::Time::Sleep(Util::Time::Seconds{1});  

            this->reqMutex.lock();
            this->responses.PushBack(std::move(response));
            this->reqMutex.unlock();

            this->connecting = false;
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

    //LoginWindow()
    //ServerBrowserWindow();
    //CreateGameWindow();
    GameWindow();
    popUp.Draw();

    // Draw GUI
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData(), guiVao.GetHandle());
    auto windowSize = client_->GetWindowSize();
    int scissor_box[4] = { 0, 0, windowSize.x, windowSize.y };
    ImGui_ImplOpenGL3_RestoreState(scissor_box);
}

void MainMenu::LoginWindow()
{
    auto displaySize = client_->io_->DisplaySize;
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    if(ImGui::Begin("Block Buster", nullptr, flags))
    {
        auto itFlags = connecting ? ImGuiInputTextFlags_::ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;
        ImGui::InputText("Username", inputUsername, 16, itFlags);

        auto winWidth = ImGui::GetWindowWidth();
        auto buttonWidth = ImGui::CalcTextSize("Login").x + 8;
        ImGui::SetCursorPosX(winWidth / 2.0f - buttonWidth / 2.0f);

        auto disabled = popUp.IsVisible();
        if(disabled)
            ImGui::PushDisabled();

        if(ImGui::Button("Login"))
        {
            Login();
        }

        if(disabled)
            ImGui::PopDisabled();
    }
    ImGui::End();
}

void MainMenu::ServerBrowserWindow()
{
    auto displaySize = ImGui::GetIO().DisplaySize;
    
    // Size
    ImGui::SetNextWindowSize(ImVec2{displaySize.x * 0.7f, displaySize.y * 0.5f}, ImGuiCond_Always);

    // Centered
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});

    auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    bool show = true;
    if(ImGui::Begin("Server Browser", &show, flags))
    {
        auto winSize = ImGui::GetWindowSize();

        auto tableSize = ImVec2{winSize.x * 0.975f, winSize.y * 0.8f};
        auto tFlags = ImGuiTableFlags_None | ImGuiTableFlags_ScrollY;
        ImGui::SetCursorPosX((winSize.x - tableSize.x) / 2.f);
        if(ImGui::BeginTable("#Server Table", 5, tFlags, tableSize))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Map");
            ImGui::TableSetupColumn("Mode");
            ImGui::TableSetupColumn("Players");
            ImGui::TableSetupColumn("Ping");
            ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible

            ImGui::TableHeadersRow();
            
            for(auto game : gamesList)
            {
                ImGui::TableNextRow();
                auto gameId = std::string(game["id"]);
                auto name = std::string(game["name"]);

                ImGui::TableNextColumn();
                auto selectFlags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick;
                if(ImGui::Selectable(name.c_str(), false, selectFlags))
                {
                    if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_::ImGuiMouseButton_Left))
                        JoinGame(gameId);
                }

                ImGui::TableNextColumn();
                ImGui::Text("Map");

                ImGui::TableNextColumn();
                ImGui::Text("Mode");

                ImGui::TableNextColumn();
                ImGui::Text("Players");

                ImGui::TableNextColumn();
                ImGui::Text("Ping");
            }

            ImGui::EndTable();
        }

        ImGui::Button("Create Game");
        ImGui::SameLine();
        ImGui::Button("Connect");
        ImGui::SameLine();
        ImGui::Button("Filters");
        ImGui::SameLine();


        if(ImGui::Button("Update"))
        {
            ListGames();
        }
    }
    if(!show)
        GetLogger()->LogInfo("No show");

    ImGui::End();
}

void MainMenu::CreateGameWindow()
{
    auto displaySize = ImGui::GetIO().DisplaySize;
    
    // Size
    //ImGui::SetNextWindowSize(ImVec2{displaySize.x * 0.7f, displaySize.y * 0.5f}, ImGuiCond_Always);

    // Centered
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});

    auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
    bool show = true;
    if(ImGui::Begin("Create Game", &show, flags))
    {
        auto textFlags = ImGuiInputTextFlags_None;
        ImGui::InputText("Name", gameName, 32, textFlags);

        if(ImGui::Button("Create Game"))
        {
            CreateGame(gameName);
        }
    }

    ImGui::End();
}

void MainMenu::GameWindow()
{
    auto displaySize = ImGui::GetIO().DisplaySize;
    
    // Size
    auto windowSize = ImVec2{displaySize.x * 0.7f, displaySize.y * 0.75f};
    //ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

    // Centered
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});

    //TODO: Check if current game has value, should go back in that case

    auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    flags = ImGuiWindowFlags_None;
    bool show = true;
    if(ImGui::Begin("Game", &show, flags))
    {
        windowSize = ImGui::GetWindowSize();

        auto layoutFlags = ImGuiTableFlags_None;
        auto layoutSize = ImVec2{windowSize.x * 0.975f, windowSize.y * 1.0f};
        if(ImGui::BeginTable("#Layout", 2, layoutFlags))
        {
            // Columns Setup
            auto colFlags = ImGuiTableColumnFlags_::ImGuiTableColumnFlags_WidthFixed;
            float leftColSize = layoutSize.x * 0.65f;
            ImGui::TableSetupColumn("#PlayerTable", colFlags, leftColSize);
            float rightColSize = 1.0f - leftColSize;
            ImGui::TableSetupColumn("#Game info", colFlags, rightColSize);

            // Player Table
            ImGui::TableNextColumn();
            auto playerTableFlags = ImGuiTableFlags_None | ImGuiTableFlags_ScrollY;
            auto playerTableSize = ImVec2{leftColSize * 0.975f, layoutSize.y * 0.5f};
            if(ImGui::BeginTable("Player List", 2, playerTableFlags, playerTableSize))
            {   
                float lvlSize = playerTableSize.x * 0.1f;
                ImGui::TableSetupColumn("Lvl", colFlags, lvlSize);
                float playerNameSize = 1.0f - lvlSize;
                ImGui::TableSetupColumn("Player Name", colFlags, playerNameSize);
                ImGui::TableSetupScrollFreeze(0, 1); // Freeze first row;

                ImGui::TableHeadersRow();

                for(auto i = 0 ; i < 8; i++)
                {
                    // Lvl Col
                    ImGui::TableNextColumn();
                    ImGui::Text("25");

                    // Player Name 
                    ImGui::TableNextColumn();
                    ImGui::Text("Defu, The Slayer");
                }
            
                ImGui::EndTable();
            }

            // Map info
            ImGui::TableNextColumn();
            ImGui::Text("Right");

            // Chat window
            ImGui::TableNextColumn();
            auto inputLineSize = ImGui::CalcTextSize("Type ");

            ImGui::Text("Chat Window");
            auto chatFlags = ImGuiInputTextFlags_ReadOnly;
            auto height = ImGui::GetCursorPosY();
            ImVec2 chatSize{playerTableSize.x, (windowSize.y - height) - inputLineSize.y * 2.0f};
            ImGui::InputTextMultiline("##Chat Window", chat, 4096, chatSize, chatFlags);

            auto chatLineFlags = ImGuiInputTextFlags_EnterReturnsTrue;
            ImGui::Text("Type"); ImGui::SameLine(); 
            ImGui::PushItemWidth(playerTableSize.x - inputLineSize.x);
            ImGui::InputText("##Type", chatLine, 128, chatLineFlags);
            ImGui::PopItemWidth();

            ImGui::EndTable();
        }
    }

    ImGui::End();
}