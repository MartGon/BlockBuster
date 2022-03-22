#include <GameState/MainMenu/MainMenu.h>

#include <Client.h>

#include <imgui/imgui_internal.h>

#include <base64/base64.h>
#include <zip/zip.h>

using namespace BlockBuster;

MainMenu::MainMenu(Client* client)  : GameState{client}, httpClient{"localhost", 3030}
{

}

MainMenu::~MainMenu()
{
}

void MainMenu::Start()
{
    menuState_ = std::make_unique<MenuState::Login>(this);

    // Set default name
    popUp.SetTitle("Connecting");
    popUp.SetFlags(ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

    SDL_SetWindowResizable(this->client_->window_, SDL_FALSE);
    client_->SetWindowSize(glm::ivec2{800, 600});
}

void MainMenu::Shutdown()
{
    // Call LeaveGame and wait for it
    if(lobby && !enteringGame)
        LeaveGame();
}

void MainMenu::Update()
{
    httpClient.HandleResponses();
    HandleSDLEvents();
    Render();
}

void MainMenu::Login(std::string userName)
{
    GetLogger()->LogDebug("Login with rest service");

    // Raise pop up
    popUp.SetTitle("Login connection");
    popUp.SetText("Connnecting to match making server");
    popUp.SetVisible(true);
    popUp.SetButtonVisible(false);

    nlohmann::json body{{"username", userName}};
    auto onSuccess = [this](httplib::Response& res){
        auto bodyStr = res.body;
        nlohmann::json body = nlohmann::json::parse(bodyStr);
        this->userId = body["id"];
        this->user = body["username"];

        popUp.SetTitle("Login succesful");
        popUp.SetText("Your username is " + std::string(body["username"]));
        popUp.SetButtonVisible(true);
        popUp.SetButtonCallback([this](){
            GetLogger()->LogInfo("Button pressed. Opening server browser");

            SetState(std::make_unique<MenuState::ServerBrowser>(this));
        });

        GetLogger()->LogInfo("Response: " + bodyStr);
        GetLogger()->Flush();
    };

    auto onError = [this](httplib::Error error){
        popUp.SetText("Could not connect to server");
        popUp.SetButtonVisible(true);

        GetLogger()->LogError("Could not connet to match-making server. Error Code: " + httplib::to_string(error));
    };
    httpClient.Request("/login", nlohmann::to_string(body), onSuccess, onError);
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
        for(auto& game : games)
        {
            auto gameInfo = GameInfo::FromJson(game);
            this->gamesList.push_back(gameInfo);
        }
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

    httpClient.Request("/list_games", nlohmann::to_string(body), onSuccess, onError);
}

void MainMenu::JoinGame(std::string gameId)
{
    GetLogger()->LogInfo("Joining game " + gameId);

    // Show connecting pop up
    popUp.SetVisible(true);
    popUp.SetCloseable(false);
    popUp.SetTitle("Connecting");
    popUp.SetText("Joining game...");
    popUp.SetButtonVisible(false);

    nlohmann::json body;
    body["player_id"] = userId;
    body["game_id"] = gameId;

    auto onSuccess = [this, gameId](httplib::Response& res)
    {
        GetLogger()->LogInfo("Succesfullly joined game " + gameId);
        GetLogger()->LogInfo("Result " + res.body);

        if(res.status == 200)
        {
            auto body = nlohmann::json::parse(res.body);
            auto gameDetails = GameDetails::FromJson(body);

            // Set current game
            currentGame = gameDetails;

            // Open game info window
            SetState(std::make_unique<MenuState::Lobby>(this));

            popUp.SetVisible(false);
        }
        else
        {
            // Handle game full errors, game doesn't exist, etc.
            std::string msg = res.body;

            popUp.SetVisible(true);
            popUp.SetText(msg);
            popUp.SetTitle("Error");
            popUp.SetButtonVisible(true);
            popUp.SetButtonCallback([this](){
                popUp.SetVisible(false);
            });
        }
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

    httpClient.Request("/join_game", nlohmann::to_string(body), onSuccess, onError);
}

void MainMenu::CreateGame(std::string name, std::string map, std::string mode, uint8_t max_players)
{
    // Create request body
    nlohmann::json body;
    body["name"] = name;
    body["player_id"] = userId;
    body["map"] = map;
    body["mode"] = mode;
    body["max_players"] = max_players;

    // Show connecting pop up
    popUp.SetVisible(true);
    popUp.SetCloseable(false);
    popUp.SetTitle("Connecting");
    popUp.SetText("Creating game...");
    popUp.SetButtonVisible(false);

    auto onSuccess = [this, name](httplib::Response& res)
    {
        auto body = nlohmann::json::parse(res.body);
        auto gameDetails = GameDetails::FromJson(body);

        // Set current game
        currentGame = gameDetails;

        // Open Lobby
        SetState(std::make_unique<MenuState::Lobby>(this));

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

    httpClient.Request("/create_game", nlohmann::to_string(body), onSuccess, onError);
}

void MainMenu::GetAvailableMaps()
{
    GetLogger()->LogInfo("Getting available maps ");
    
    // Show connecting pop up
    popUp.SetVisible(true);
    popUp.SetCloseable(false);
    popUp.SetTitle("Connecting");
    popUp.SetText("Retrieving data from server...");
    popUp.SetButtonVisible(false);

    auto onSuccess = [this](httplib::Response& res)
    {
        GetLogger()->LogInfo("Succesfully retrieved available maps");

        auto bodyStr = res.body;        
        auto body = nlohmann::json::parse(bodyStr);

        auto maps = body.at("maps");
        this->availableMaps.clear();
        for(auto& map : maps)
        {
            auto mapName = map.get<std::string>();
            this->availableMaps.push_back(mapName);
        }

        SetState(std::make_unique<MenuState::CreateGame>(this));

        popUp.SetVisible(false);
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

    nlohmann::json body;
    httpClient.Request("/get_available_maps", nlohmann::to_string(body), onSuccess, onError);
}

void MainMenu::LeaveGame()
{
    // Show connecting pop up
    popUp.SetVisible(true);
    popUp.SetCloseable(true);
    popUp.SetTitle("Connecting");
    popUp.SetText("Leaving game...");
    popUp.SetButtonVisible(false);

    nlohmann::json body;
    body["player_id"] = userId;

    auto onSuccess = [this](httplib::Response& res)
    {
        GetLogger()->LogInfo("Succesfullly left game");

        // Open Server Browser
        SetState(std::make_unique<MenuState::ServerBrowser>(this));

        // Remove current game;
        this->currentGame.reset();

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

    httpClient.Request("/leave_game", nlohmann::to_string(body), onSuccess, onError);
}

void MainMenu::ToggleReady()
{
    if(!lobby)
        return;

    nlohmann::json body;
    body["player_id"] = userId;

    auto onSuccess = [this](httplib::Response& res)
    {
        GetLogger()->LogInfo("Succesfully set ready state");
        GetLogger()->LogInfo("Result " + res.body);
    };

    auto onError = [this](httplib::Error err)
    {
        auto errorCode = static_cast<int>(err);
        GetLogger()->LogError("Could not toggle ready. Error Code: " + std::to_string(errorCode));
    };

    httpClient.Request("/toggle_ready", nlohmann::to_string(body), onSuccess, onError);
}

void MainMenu::SendChatMsg(std::string msg)
{
    if(!lobby)
        return;

    nlohmann::json body;
    body["player_id"] = userId;
    body["msg"] = msg;

    auto onSuccess = [this](httplib::Response& res)
    {
        if(res.status == 200)
            GetLogger()->LogInfo("Succesfully sent chat msg");
    };

    auto onError = [this](httplib::Error err)
    {
        auto errorCode = static_cast<int>(err);
        GetLogger()->LogError("Could not send chat msg. Error Code: " + std::to_string(errorCode));
    };

    httpClient.Request("/send_chat_msg", nlohmann::to_string(body), onSuccess, onError);
}

void MainMenu::UpdateGame()
{
    // Only update current game while in lobby
    if(lobby && lobby->updatePending)
        return;

    nlohmann::json body;
    body["game_id"] = currentGame->game.id;

    auto onSuccess = [this](httplib::Response& res)
    {
        if(lobby)
            lobby->updatePending = false;
            
        GetLogger()->LogInfo("[Update Game]Result " + res.body);

        if(res.status == 200)
        {
            GetLogger()->LogInfo("Succesfully updated game");
            auto body = nlohmann::json::parse(res.body);
            auto gameDetails = GameDetails::FromJson(body);

            // Check if we are still in lobby. Keep updating in that case
            if(lobby)
            {
                auto oldState = currentGame->game.state;

                // Set current game
                currentGame = gameDetails;

                if(currentGame->game.state == "InGame" && oldState == "InLobby")
                {
                    GetLogger()->LogInfo("Game Started!");
                    auto address = currentGame->game.address.value();
                    auto port = currentGame->game.serverPort.value();
                    client_->LaunchGame(address, port, gameDetails.game.map);

                    httpClient.Disable();
                    enteringGame = true;
                }
                else if(currentGame->game.state == "InLobby")
                {
                    // Update chat
                    lobby->OnGameInfoUpdate();

                    // Call again
                    UpdateGame();
                }
            }
        }
        // Handle game doesn't exist, etc.
        else
        {
            GetLogger()->LogError("[Update Game]: Game didn't exist");
        }
    };

    auto onError = [this](httplib::Error err)
    {
        lobby->updatePending = false;
        auto errorCode = static_cast<int>(err);
        GetLogger()->LogError("Could not update game with id " + this->currentGame->game.id + ". Error Code: " + std::to_string(errorCode));

        // Timeout on read. Call again
        if(err == httplib::Error::Read)
        {
            UpdateGame();
        }
    };

    lobby->updatePending = true;
    httpClient.Request("/update_game", nlohmann::to_string(body), onSuccess, onError);
}

void MainMenu::StartGame()
{
    if(!lobby)
        return;
    
    nlohmann::json body;
    body["game_id"] = currentGame->game.id;
    body["player_id"] = userId;

    auto onSuccess = [this](httplib::Response& res)
    {
        if(res.status == 200)
            GetLogger()->LogInfo("Succesfully sent start game");
    };

    auto onError = [this](httplib::Error err)
    {
        auto errorCode = static_cast<int>(err);
        
        GetLogger()->LogError("Could not start game. Error Code: " + std::to_string(errorCode));
        httpClient.Enable();
    };

    httpClient.Request("/start_game", nlohmann::to_string(body), onSuccess, onError);
    httpClient.Disable();
}

void MainMenu::DownloadMap(std::string mapName)
{
    GetLogger()->LogInfo("Downloading map: " + mapName);

    // Show connecting pop up
    popUp.SetVisible(true);
    popUp.SetCloseable(false);
    popUp.SetTitle("Connecting");
    popUp.SetText("Downloading game map...");
    popUp.SetButtonVisible(false);

    nlohmann::json body;
    body["map_name"] = mapName;

    auto onSuccess = [this, mapName](httplib::Response& res)
    {
        GetLogger()->LogInfo("Succesfullly downloaded map " + mapName);
        GetLogger()->LogInfo("Result " + res.body);

        if(res.status == 200)
        {
            auto body = nlohmann::json::parse(res.body);
            auto mapB64 = body["map"].get<std::string>();
            auto mapBuff = base64_decode(mapB64);

            auto mapFolder = client_->mapMgr.GetMapsFolder() / mapName;
            std::filesystem::create_directory(mapFolder);
            GetLogger()->LogInfo("Creating dir " + mapFolder.string());
            zip_stream_extract(mapBuff.data(), mapBuff.size(), mapFolder.c_str(), nullptr, nullptr);

            popUp.SetVisible(false);
        }
        else
        {
            // Handle game full errors, game doesn't exist, etc.
            std::string msg = res.body;

            popUp.SetVisible(true);
            popUp.SetText(msg);
            popUp.SetTitle("Error");
            popUp.SetButtonVisible(true);
            popUp.SetButtonCallback([this](){
                popUp.SetVisible(false);
            });
        }
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

     httpClient.Request("/download_map", nlohmann::to_string(body), onSuccess, onError);
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

    menuState_->Update();
    popUp.Draw();

    // Draw GUI
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData(), guiVao.GetHandle());
    auto windowSize = client_->GetWindowSize();
    int scissor_box[4] = { 0, 0, windowSize.x, windowSize.y };
    ImGui_ImplOpenGL3_RestoreState(scissor_box);
}

void MainMenu::SetState(std::unique_ptr<MenuState::Base> menuState)
{
    this->menuState_->OnExit();

    this->menuState_ = std::move(menuState);

    this->menuState_->OnEnter();
}

MapMgr& MainMenu::GetMapMgr()
{
    return client_->mapMgr;
}