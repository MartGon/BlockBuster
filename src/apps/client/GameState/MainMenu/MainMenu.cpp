#include <GameState/MainMenu/MainMenu.h>

#include <Client.h>
#include <util/Zip.h>
#include <Match.h>

#include <imgui/imgui_internal.h>
#include <base64/base64.h>

#include <util/Container.h>

using namespace BlockBuster;

MainMenu::MainMenu(Client* client)  : GameState{client}, httpClient{"localhost", 3030}, 
    menuState_{std::make_unique<MenuState::Login>(this)}
{

}

MainMenu::~MainMenu()
{
}

void MainMenu::Start()
{
    // Set default name
    popUp.SetTitle("Connecting");
    popUp.SetFlags(ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

    // Reset window
    ResetWindow();

    // Start camera
    auto winSize = client_->GetWindowSize();
    camera_.SetPos(glm::vec3{0.0f});
    camera_.SetTarget(glm::vec3{0.0f, 0.0f, -1.0f});
    camera_.SetParam(Rendering::Camera::Param::ASPECT_RATIO, (float)winSize.x / (float)winSize.y);
    camera_.SetParam(Rendering::Camera::Param::FOV, client_->config.window.fov);

    // Set matchmaking address and port
    auto address = client_->GetConfigOption("MatchMakingServerAddress", "127.0.0.1");
    uint16_t port = std::atoi(client_->GetConfigOption("MatchMakinServerPort", "3030").c_str());
    httpClient.SetAddress(address, port);
}

void MainMenu::Shutdown()
{
    // Call LeaveGame and wait for it
    if(lobby && !enteringGame)
        LeaveGame();

    httpClient.Close();

    // Write config
    client_->config.options["MatchMakingServerAddress"] = httpClient.GetAddress().first;
    client_->config.options["MatchMakinServerPort"] = std::to_string(httpClient.GetAddress().second);
}

void MainMenu::Update()
{
    auto now = Util::Time::GetTime();

    httpClient.HandleResponses();
    HandleSDLEvents();
    Render();
    
    auto afterNow = Util::Time::GetTime();
    deltaTime = afterNow - now;
}

void MainMenu::Login(std::string userName)
{
    GetLogger()->LogDebug("Login with rest service");

    // Raise pop up
    RaisePopUp("Login connection", "Connecting to match-making server");

    nlohmann::json body{{"username", userName}};
    auto onSuccess = [this](httplib::Response& res){
        auto bodyStr = res.body;
        nlohmann::json body = nlohmann::json::parse(bodyStr);
        this->userId = body["id"];
        this->user = body["username"];

        auto callback = [this](){
            GetLogger()->LogDebug("Button pressed. Opening server browser");

            SetState(std::make_unique<MenuState::ServerBrowser>(this));
        };
        RaisePopUp("Login succesful", "Your username is " + std::string(body["username"]), false, true, callback);

        GetLogger()->LogDebug("Response: " + bodyStr);
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
    GetLogger()->LogDebug("Requesting list of games");

    nlohmann::json body;
    auto onSuccess = [this](httplib::Response& res)
    {
        GetLogger()->LogDebug("List of games updated successfully");

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
        RaisePopUp("Error", "Connection Error", true, true);
    };

    httpClient.Request("/list_games", nlohmann::to_string(body), onSuccess, onError);
}

void MainMenu::JoinGame(std::string gameId)
{
    GetLogger()->LogDebug("Joining game " + gameId);

    // Show connecting pop up
    RaisePopUp("Connecting", "Joining game...");

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
            RaisePopUp("Error", res.body, true, true);
        }
    };

    auto onError = [this](httplib::Error err)
    {
        RaisePopUp("Error", "Connection Error", true, true);
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
    RaisePopUp("Connecting", "Creating game...");

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
        RaisePopUp("Error", "Connection Error", true, true);
    };

    httpClient.Request("/create_game", nlohmann::to_string(body), onSuccess, onError);
}


void MainMenu::EditGame(std::string name, std::string map, std::string mode)
{
    // Create request body
    nlohmann::json body;
    body["player_id"] = userId;
    body["game_id"] = currentGame->game.id;
    body["name"] = name;
    body["map"] = map;
    body["mode"] = mode;

    auto onSuccess = [this, name](httplib::Response& res)
    {   
        if(res.status == 200)
        {
            auto body = nlohmann::json::parse(res.body);
            auto gameDetails = GameDetails::FromJson(body);

            // Force update
            UpdateGame(true);
        }
        else
        {
            RaisePopUp("Error", res.body, true, true);
        }
    };

    auto onError = [this](httplib::Error err)
    {
        RaisePopUp("Error", "Connection Error", true, true);
    };

    httpClient.Request("/edit_game", nlohmann::to_string(body), onSuccess, onError);
}

void MainMenu::GetAvailableMaps()
{
    GetLogger()->LogDebug("Getting available maps ");
    
    // Show connecting pop up
    RaisePopUp("Connecting", "Retrieving data from server...");

    auto onSuccess = [this](httplib::Response& res)
    {
        GetLogger()->LogInfo("Succesfully retrieved available maps");

        auto bodyStr = res.body;        
        auto body = nlohmann::json::parse(bodyStr);

        auto maps = body.at("maps");
        this->availableMaps.clear();
        for(auto& map : maps)
        {
            auto mapInfo = MapInfo::FromJson(map);
            this->availableMaps.push_back(mapInfo);
        }

        popUp.SetVisible(false);
    };

    auto onError = [this](httplib::Error error){
        RaisePopUp("Error", "Connection Error", true, true);
    };

    nlohmann::json body;
    httpClient.Request("/get_available_maps", nlohmann::to_string(body), onSuccess, onError);
}

void MainMenu::LeaveGame()
{
    // Show connecting pop up
    RaisePopUp("Connecting", "Leaving game...");

    nlohmann::json body;
    body["player_id"] = userId;

    auto onSuccess = [this](httplib::Response& res)
    {
        GetLogger()->LogInfo("Succesfullly left game");

        // Open Server Browser
        SetState(std::make_unique<MenuState::ServerBrowser>(this));

        // Remove current game;
        this->currentGame.reset();
        this->lobby = nullptr;

        popUp.SetVisible(false);
    };

    auto onError = [this](httplib::Error err)
    {
        RaisePopUp("Error", "Connection Error", true, true);
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
        GetLogger()->LogDebug("Result " + res.body);

        // Force update
        UpdateGame(true);
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

void MainMenu::UpdateGame(bool forced)
{
    // Only update current game while in lobby
    if(lobby && lobby->updatePending)
        return;

    nlohmann::json body;
    body["game_id"] = currentGame->game.id;
    body["forced"] = forced;

    auto onSuccess = [this](httplib::Response& res)
    {
        if(lobby)
            lobby->updatePending = false;
            
        GetLogger()->LogDebug("[Update Game]Result " + res.body);

        if(res.status == 200)
        {
            GetLogger()->LogDebug("Succesfully updated game");
            auto body = nlohmann::json::parse(res.body);
            auto gameDetails = GameDetails::FromJson(body);

            // Check if we are still in lobby. Keep updating in that case
            if(lobby)
            {
                auto oldGameInfo = currentGame->game;
                // Set current game
                currentGame = gameDetails;

                // On map change
                auto mapName = currentGame->game.map;
                if(mapName != oldGameInfo.map)
                {
                    if(ShouldDownloadMap(mapName))
                    {
                        GetLogger()->LogWarning("Need to download map " + mapName);
                        DownloadMap(mapName);
                    }
                    GetMapPicture(mapName);
                }

                // Check game start
                auto oldState = oldGameInfo.state;
                if(currentGame->game.state == "InGame" && oldState == "InLobby")
                {
                    LaunchGame();
                }
                else if(currentGame->game.state == "InLobby")
                {
                    // Update chat
                    lobby->OnGameInfoUpdate();
                }
            }
        }
        // Handle game doesn't exist, etc.
        else if(lobby)
        {
            GetLogger()->LogError("[Update Game]: Game didn't exist");

            // Show error popUp
            RaisePopUp("Error", res.body, true, true);

            // Back to lobby
            SetState(std::make_unique<MenuState::ServerBrowser>(this));
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
        
        }
    };

    lobby->updatePending = true;
    httpClient.Request("/update_game", nlohmann::to_string(body), onSuccess, onError);
}

void MainMenu::StartGame()
{
    if(!lobby)
        return;

    GetLogger()->LogDebug("Start Game req sent");
    
    nlohmann::json body;
    body["game_id"] = currentGame->game.id;
    body["player_id"] = userId;

    auto onSuccess = [this](httplib::Response& res)
    {
        if(res.status == 200)
        {
            GetLogger()->LogInfo("Succesfully sent start game");

            // Force Update
            UpdateGame(true);
        }
        else
        {
            RaisePopUp("Error", res.body, true, true);
        }
    };

    auto onError = [this](httplib::Error err)
    {
        auto errorCode = static_cast<int>(err);
        
        GetLogger()->LogError("Could not start game. Error Code: " + std::to_string(errorCode));
        httpClient.Enable();
    };

    httpClient.Request("/start_game", nlohmann::to_string(body), onSuccess, onError);
}

void MainMenu::DownloadMap(std::string mapName)
{
    GetLogger()->LogDebug("Downloading map: " + mapName);

    // Show connecting pop up
    RaisePopUp("Connecting", "Downloading game map...");

    nlohmann::json body;
    body["map_name"] = mapName;

    auto onSuccess = [this, mapName](httplib::Response& res)
    {
        GetLogger()->LogInfo("Succesfullly downloaded map " + mapName);

        if(res.status == 200)
        {
            auto body = nlohmann::json::parse(res.body);
            auto mapB64 = body["map"].get<std::string>();
            auto mapBuff = base64_decode(mapB64);

            auto mapsFolder = client_->mapMgr.GetMapsFolder();
            zip_stream_extract(mapBuff.data(), mapBuff.size(), mapsFolder.string().c_str(), nullptr, nullptr);

            RaisePopUp("Success", mapName + " was downloaded succesfully!", true, true);
        }
        else
        {
            RaisePopUp("Error", res.body, true, true);
        }
    };

    auto onError = [this](httplib::Error err)
    {
        RaisePopUp("Error", "Connection Error", true, true);
    };

     httpClient.Request("/download_map", nlohmann::to_string(body), onSuccess, onError);
}

void MainMenu::GetMapPicture(std::string mapName)
{
    if(Util::Map::Contains(mapPics, mapName))
    {
        GetLogger()->LogInfo("Did not download map picture: " + mapName + " because it was already cached");    
        return;
    }

    GetLogger()->LogDebug("Downloading map picture: " + mapName);

    nlohmann::json body;
    body["map_name"] = mapName;

    auto onSuccess = [this, mapName](httplib::Response& res)
    {
        GetLogger()->LogInfo("Succesfullly downloaded map picture: " + mapName);

        if(res.status == 200)
        {
            auto body = nlohmann::json::parse(res.body);
            auto mapPictureB64 = body["map_picture"].get<std::string>();
            auto mapPictureBuff = base64_decode(mapPictureB64);

            // Load and cache texture
            GL::Texture texture;
            texture.LoadFromMemory(mapPictureBuff.data(), mapPictureBuff.size());

            ImGui::Impl::Texture mapPicImpl;
            mapPicImpl.handle = texture.GetGLId();
            mapPicImpl.type = ImGui::Impl::TextureType::SINGLE_TEXTURE;

            MapPic mapPic;
            mapPic.texture = std::move(texture);
            mapPic.imGuiImpl = mapPicImpl;

            mapPics[mapName] = std::move(mapPic);
        }
        else
        {
            RaisePopUp("Error", res.body, true, true);
        }
    };

    auto onError = [this](httplib::Error err)
    {
        RaisePopUp("Error", "Connection Error", true, true);
    };

    httpClient.Request("/get_map_picture", nlohmann::to_string(body), onSuccess, onError);
}

void MainMenu::UploadMap(std::string mapName, std::string password)
{
    // Write map version
    // TODO: This could be done in editor, by hashing the map binary file, or something
    auto version = Util::Random::RandomString(16);
    client_->mapMgr.WriteMapVersion(mapName, version);

    // Payload
    nlohmann::json body;
    body["map_name"] = mapName;
    body["password"] = password;
    body["map_version"] = version;
        
    auto mapFile = client_->mapMgr.GetMapFile(mapName);
    auto res = Game::Map::Map::LoadFromFile(mapFile);
    if(res.isOk())
    {  
        // Supported gamemodes
        auto mapPtr = std::move(res.unwrap());
        auto map = std::move(*mapPtr.get());
        body["supported_gamemodes"] = BlockBuster::GetSupportedGameModesAsString(map);

            // Create map payload
        Util::ZipStream zipStream{'w'};
        auto mapFolder = client_->mapMgr.GetMapFolder(mapName);
        zipStream.ZipFolder(mapFolder);
        auto buffer = zipStream.GetBuffer();
        body["map_zip"] = base64_encode(buffer);

        auto onSuccess = [this, mapName](httplib::Response& res)
        {
            GetLogger()->LogInfo("Succesfullly uploaded map " + mapName);
            GetLogger()->LogDebug("Result " + res.body);

            if(res.status == 200)
            {
                
                auto cb = [this](){
                    popUp.SetVisible(false);
                    SetState(std::make_unique<MenuState::ServerBrowser>(this));
                };
                RaisePopUp("Success", mapName + " was uploaded succesfully!", false, true, cb);
            }
            else
            {
                std::string msg = res.body;
                RaisePopUp("Error", msg, true, true);
            }
        };

        auto onError = [this](httplib::Error err)
        {
            RaisePopUp("Error", "Connection Error", true, true);
        };

        RaisePopUp("Connecting", "Uploading map...", true, true);

        httpClient.Request("/upload_map", nlohmann::to_string(body), onSuccess, onError);
    }
    else
    {
        this->RaisePopUp("Error", "Could not uplaod map", true);

        GetLogger()->LogError("Could not load map " + mapName);
    }
    return;
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

    DrawSkybox();
    DrawGUI();

    // Swap buffers
    SDL_GL_SwapWindow(client_->window_);
}

void MainMenu::DrawSkybox()
{
    auto now = Util::Time::GetTime().time_since_epoch();
    auto secs = Util::Time::Seconds(now);
    const auto ROT_SPEED = 0.005f;
    auto yaw = std::sin(secs.count() * ROT_SPEED) * glm::two_pi<float>();
    auto rot = camera_.GetRotation();
    camera_.SetRotation(rot.x, yaw);

    auto view = camera_.GetViewMat();
    auto proj = camera_.GetProjMat();

    client_->skybox.Draw(client_->skyboxShader, view, proj, true);
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

void MainMenu::ResetWindow()
{
    // Set window properties
    client_->SetMouseGrab(false);
    SDL_SetWindowFullscreen(this->client_->window_, 0);
    
    client_->SetWindowSize(glm::ivec2{800, 600});
    SDL_SetWindowPosition(this->client_->window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_SetWindowResizable(this->client_->window_, SDL_FALSE);

    // Reset flag
    enteringGame = false;
}

void MainMenu::SetState(std::unique_ptr<MenuState::Base> menuState)
{
    this->menuState_->OnExit();

    this->menuState_ = std::move(menuState);

    this->menuState_->OnEnter();
}

void MainMenu::RaisePopUp(std::string title, std::string text, bool closeable, bool showButton, std::function<void()> onClick)
{
    popUp.SetVisible(true);
    
    popUp.SetTitle(title);
    popUp.SetText(text);

    popUp.SetCloseable(closeable);
    popUp.SetButtonVisible(showButton);
    auto f = [this](){
        popUp.SetVisible(false);
    };
    if(onClick)
        popUp.SetButtonCallback(onClick);
    else
        popUp.SetButtonCallback(f);
}

// Handy

void MainMenu::LaunchGame()
{
    GetLogger()->LogDebug("Game Started!");
    auto address = currentGame->game.address.value();
    auto port = currentGame->game.serverPort.value();
    client_->LaunchGame(address, port, currentGame->game.map, userId, user);

    //httpClient.Disable();
    enteringGame = true;
}

MapMgr& MainMenu::GetMapMgr()
{
    return client_->mapMgr;
}

bool MainMenu::ShouldDownloadMap(std::string map)
{
    auto& mapMgr = GetMapMgr();
    auto serverVersion = currentGame->game.map_version;

    bool hasMap = mapMgr.HasMap(map);
    return !hasMap || mapMgr.ReadMapVersion(map) != serverVersion;
}
