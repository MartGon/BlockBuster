#include <GameState/MainMenu/MenuState.h>
#include <GameState/MainMenu/MainMenu.h>

#include <Client.h>

#include <imgui.h>
#include <imgui_internal.h>

using namespace BlockBuster::MenuState;

Base::Base(MainMenu* mainMenu) : mainMenu_{mainMenu}
{

}

// #### LOGIN #### \\

void Login::Update()
{
    auto displaySize = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    if(ImGui::Begin("Block Buster", nullptr, flags))
    {
        int itFlags = mainMenu_->httpClient.IsConnecting() ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;
        itFlags |= ImGuiInputTextFlags_EnterReturnsTrue;
        bool enter = ImGui::InputText("Username", inputUsername, 16, itFlags);

        auto winWidth = ImGui::GetWindowWidth();
        auto buttonWidth = ImGui::CalcTextSize("Login").x + 8;
        ImGui::SetCursorPosX(winWidth / 2.0f - buttonWidth / 2.0f);

        auto disabled = mainMenu_->popUp.IsVisible();
        if(disabled)
            ImGui::PushDisabled();

        if(ImGui::Button("Login") || enter)
        {
            mainMenu_->Login(inputUsername);
        }

        if(disabled)
            ImGui::PopDisabled();
    }
    ImGui::End();
}

// #### SERVER BROWSER #### \\

void ServerBrowser::OnEnter()
{
    mainMenu_->ListGames();
}

void ServerBrowser::Update()
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
        auto tFlags = ImGuiTableFlags_None | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg;
        ImGui::SetCursorPosX((winSize.x - tableSize.x) / 2.f);
        if(ImGui::BeginTable("#Server Table", 6, tFlags, tableSize))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Map");
            ImGui::TableSetupColumn("Mode");
            ImGui::TableSetupColumn("Players");
            ImGui::TableSetupColumn("State");
            ImGui::TableSetupColumn("Ping");
            ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible

            ImGui::TableHeadersRow();
            
            for(auto game : mainMenu_->gamesList)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                auto gameId = game.id;
                auto name = game.name;

                auto selectFlags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick;
                if(ImGui::Selectable(name.c_str(), false, selectFlags))
                {
                    if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_::ImGuiMouseButton_Left))
                        mainMenu_->JoinGame(gameId);
                }

                ImGui::TableNextColumn();
                ImGui::Text("%s", game.map.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", game.mode.c_str());

                ImGui::TableNextColumn();
                auto players = std::to_string(game.players) + "/" + std::to_string(game.maxPlayers);
                ImGui::Text("%s", players.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", game.state.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%i", game.ping);
            }

            ImGui::EndTable();
        }

        auto buttonSize = ImGui::CalcTextSize(" Create Game ");
        ImGui::SetCursorPosY(winSize.y - (buttonSize.y * 2 + 4));
        if(ImGui::Button("Create Game"))
        {
            mainMenu_->GetAvailableMaps();
            mainMenu_->SetState(std::make_unique<MenuState::CreateGame>(mainMenu_));
        }

        ImGui::SameLine();
        if(ImGui::Button("Update"))
        {
            mainMenu_->ListGames();
        }

        /*
        ImGui::SameLine();
        ImGui::Button("Connect");
        ImGui::SameLine();
        ImGui::Button("Filters");
        */

        ImGui::SameLine();
        if(ImGui::Button("Upload Map"))
        {
            mainMenu_->SetState(std::make_unique<UploadMap>(mainMenu_));
        }
    }
    // User clicked on the X button. Go back to login page
    if(!show)
        mainMenu_->SetState(std::make_unique<MenuState::Login>(mainMenu_));

    ImGui::End();
}

// #### CREATE GAME #### \\

void CreateGame::OnEnter()
{
    std::string placeholderName = mainMenu_->user + "'s game";
    strcpy(gameName, placeholderName.c_str());
    if(!mainMenu_->availableMaps.empty())
        SelectMap(mainMenu_->availableMaps[0]);
}

void CreateGame::Update()
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
        auto& mapPic = mainMenu_->mapPics[mapInfo.mapName];
        if(mapPic.texture.IsLoaded())
        {   
            auto size = mapPic.texture.GetSize();
            GUI::ImGuiImage(&mapPic.imGuiImpl, size);
        }

        auto textFlags = ImGuiInputTextFlags_None;
        ImGui::InputText("Name", gameName, 32, textFlags);

        auto comboFlags = ImGuiComboFlags_None;
        if(ImGui::BeginCombo("Map", mapInfo.mapName.c_str(), comboFlags))
        {   
            for(auto availMap : mainMenu_->availableMaps)
            {
                bool selected = availMap.mapName == this->mapInfo.mapName;
                if(ImGui::Selectable(availMap.mapName.c_str(), selected))
                {
                    SelectMap(availMap);
                }
            }
            ImGui::EndCombo();
        }

        if(ImGui::BeginCombo("Mode", mode.c_str(), comboFlags))
        {
            for(auto gameMode : mapInfo.supportedGamemodes)
            {
                bool selected = gameMode == this->mode;
                if(ImGui::Selectable(gameMode.c_str(), selected))
                {
                    this->mode = gameMode;
                }
            }
            ImGui::EndCombo();
        }

        auto sliderFlags = ImGuiSliderFlags_::ImGuiSliderFlags_None;
        ImGui::SliderInt("Max players", &maxPlayers, 2, 16, "%i", sliderFlags);

        bool disabled = mapInfo.mapName.empty() || mode.empty() || strlen(gameName) == 0;
        if(disabled)
            ImGui::PushDisabled();

        if(ImGui::Button("Create Game"))
        {
            mainMenu_->CreateGame(gameName, mapInfo.mapName, this->mode, maxPlayers);
        }

        if(disabled)
            ImGui::PopDisabled();
    }
    // User clicked on the X button. Go back
    if(!show)
        mainMenu_->SetState(std::make_unique<MenuState::ServerBrowser>(mainMenu_));

    ImGui::End();
}

void CreateGame::SelectMap(MapInfo a)
{
    this->mapInfo = a;
    if(!a.supportedGamemodes.empty())
        this->mode = a.supportedGamemodes[0];
    
    mainMenu_->GetMapPicture(a.mapName);
}

// #### LOBBY #### \\

void Lobby::OnEnter()
{
    // Get maps, in case we becom host
    mainMenu_->GetAvailableMaps();

    // Int strings
    gameName = mainMenu_->currentGame->game.name;
    mode = mainMenu_->currentGame->game.mode;
    gameName.resize(32);

    auto mapName = mainMenu_->currentGame->game.map;
    // Init map info
    for(auto map : mainMenu_->availableMaps)
    {
        if(map.mapName == mapName)
            this->mapInfo = map;
    }

    // Set lobby
    mainMenu_->lobby = this;

    // Do we download map?
    if(mainMenu_->ShouldDownloadMap(mapName))
    {
        mainMenu_->GetLogger()->LogWarning("Need to download map " + mapName);
        mainMenu_->DownloadMap(mapName);
    }

    // Start
    OnGameInfoUpdate();
    mainMenu_->GetMapPicture(mapName);
    mainMenu_->UpdateGame(true);
    reqTimer.Start();
}

void Lobby::OnExit()
{
    mainMenu_->lobby = nullptr;
}

void Lobby::OnGameInfoUpdate()
{
    auto mapName = mainMenu_->currentGame->game.map;
    mode = mainMenu_->currentGame->game.mode;
    for(auto map : mainMenu_->availableMaps)
    {
        if(map.mapName == mapName)
            this->mapInfo = map;
    }

    auto chatData = mainMenu_->currentGame->game.chat;
    char* chatPtr = this->chat;
    for(auto it = chatData.begin(); it != chatData.end(); it++)
    {
        auto lineSize = it->size();
        auto size = chatPtr - this->chat + lineSize;
        if(size < 4096 /*Chat Size*/)
        {
            strcpy(chatPtr, it->c_str());
            chatPtr += lineSize;
        }
    }
}

void Lobby::Update()
{
    DrawWindow();

    reqTimer.Update(mainMenu_->deltaTime);
    if(reqTimer.IsDone())
    {
        mainMenu_->GetLogger()->LogError("Sending periodic update");
        reqTimer.Restart();
        mainMenu_->UpdateGame(true);
    }
}

void Lobby::DrawWindow()
{
    auto displaySize = ImGui::GetIO().DisplaySize;
    
    // Size
    auto windowSize = ImVec2{displaySize.x * 0.85f, displaySize.y * 0.75f};
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

    // Centered
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});

    // CurrentGame should have a value, in order to operate correctly
    bool show = true;
    if(!mainMenu_->currentGame.has_value())
    {
        mainMenu_->GetLogger()->LogError("Current game didn't have a value. Returning to server browser");
        show = false;
    }

    GameDetails gameDetails = mainMenu_->currentGame.value();
    auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
    if(ImGui::Begin(gameDetails.game.name.c_str(), &show, flags))
    {
        windowSize = ImGui::GetWindowSize();

        auto layoutFlags = ImGuiTableFlags_None;
        auto layoutSize = ImVec2{windowSize.x * 0.975f, windowSize.y * 1.0f};
        if(ImGui::BeginTable("#Layout", 2, layoutFlags))
        {
            // Columns Setup
            auto colFlags = ImGuiTableColumnFlags_::ImGuiTableColumnFlags_WidthFixed;
            float leftColSize = layoutSize.x * 0.60f;
            ImGui::TableSetupColumn("#PlayerTable", colFlags, leftColSize);
            float rightColSize = 1.0f - leftColSize;
            ImGui::TableSetupColumn("#Game info", colFlags, rightColSize);

            // Player Table
            ImGui::TableNextColumn();
            auto playerTableFlags = ImGuiTableFlags_None | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg;
            auto playerTableSize = ImVec2{leftColSize * 0.975f, layoutSize.y * 0.5f};
            if(ImGui::BeginTable("Player List", 2, playerTableFlags, playerTableSize))
            {   
                float lvlSize = 0.1f * playerTableSize.x;
                float readySize = 0.3f * playerTableSize.x;
                float playerNameSize = playerTableSize.x - lvlSize - readySize;
                ImGui::TableSetupColumn("Player Name", colFlags, playerNameSize);
                ImGui::TableSetupColumn("Status", colFlags, readySize);
                ImGui::TableSetupScrollFreeze(0, 1); // Freeze first row;

                ImGui::TableHeadersRow();

                // Print player's info
                for(auto playerInfo : mainMenu_->currentGame->playersInfo)
                {
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", playerInfo.playerName.c_str());

                    ImGui::TableNextColumn();
                    
                    std::string status;
                    if(playerInfo.isHost)
                        status = "Host";
                    else
                        status = playerInfo.isReady ? "Ready" : "Not ready";
                    ImGui::Text("%s", status.c_str());
                }
            
                ImGui::EndTable();
            }

            // Map Picture
            auto gameInfo = gameDetails.game;
            ImGui::TableNextColumn();
            ImGui::Text("Map: %s", gameInfo.map.c_str());

            auto width = ImGui::GetContentRegionAvail().x;
            ImVec2 imgSize{width, layoutSize.y * 0.5f};
            auto& mapPic = mainMenu_->mapPics[gameInfo.map];
            if(mapPic.texture.IsLoaded())
            {   
                auto size = mapPic.texture.GetSize();
                GUI::ImGuiImage(&mapPic.imGuiImpl, size);
            }
            //ImGui::Dummy(imgSize);

            // Chat window
            ImGui::TableNextColumn();
            auto inputLineSize = ImGui::CalcTextSize("Type ");

            ImGui::Text("Chat Window");
            auto chatFlags = ImGuiInputTextFlags_ReadOnly;
            auto height = ImGui::GetCursorPosY();
            float marginSize = 4.0f;
            ImVec2 chatSize{playerTableSize.x, (layoutSize.y - height) - inputLineSize.y * 2.0f - marginSize};

            ImGui::InputTextMultiline("##Chat Window", this->chat, 256, chatSize, chatFlags);

            auto chatLineFlags = ImGuiInputTextFlags_EnterReturnsTrue;
            ImGui::Text("Type"); ImGui::SameLine(); 
            ImGui::PushItemWidth(playerTableSize.x - inputLineSize.x);
            if(ImGui::InputText("##Type", chatLine, 128, chatLineFlags))
            {
                mainMenu_->SendChatMsg(chatLine);
                chatLine[0] = '\0';
            }
            ImGui::PopItemWidth();

            // Map Info
            bool isHost = IsPlayerHost();
            bool changes = false;

            ImGui::TableNextColumn();
            ImGui::Text("Game Info");

            auto gitFlags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV;
            ImVec2 gitSize{layoutSize.x - leftColSize, 0};
            if(ImGui::BeginTable("Game Info", 1, gitFlags, gitSize))
            {
                if(!isHost)
                    ImGui::PushDisabled();

                // Game's Name
                auto inputTextFlags = !isHost ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;
                ImGui::TableNextColumn();
                ImGui::InputText("Name", gameName.data(), gameName.size(), inputTextFlags);
                ImGui::SameLine();
                changes = ImGui::Button("OK");

                // Game's Map
                ImGui::TableNextColumn();
                auto comboFlags = ImGuiComboFlags_None;
                if(ImGui::BeginCombo("Map", mapInfo.mapName.c_str(), comboFlags))
                {   
                    for(auto availMap : mainMenu_->availableMaps)
                    {
                        bool selected = availMap.mapName == this->mapInfo.mapName;
                        if(ImGui::Selectable(availMap.mapName.c_str(), selected))
                        {
                            SelectMap(availMap);
                            changes |= true;
                        }
                    }
                    ImGui::EndCombo();
                };

                // Game's Mode
                ImGui::TableNextColumn();
                if(ImGui::BeginCombo("Mode", mode.c_str(), comboFlags))
                {
                    for(auto gameMode : mapInfo.supportedGamemodes)
                    {
                        bool selected = gameMode == this->mode;
                        if(ImGui::Selectable(gameMode.c_str(), selected))
                        {
                            this->mode = gameMode;
                            changes |= true;
                        }
                    }
                    ImGui::EndCombo();
                }

                // Max players
                ImGui::TableNextColumn();
                int players = gameInfo.maxPlayers;
                ImGui::SliderInt("Max Players", &players, 2, 16, "%d", ImGuiInputTextFlags_ReadOnly);

                if(!isHost)
                    ImGui::PopDisabled();

                ImGui::TableNextColumn();
                ImGui::EndTable();
            }

            if(changes)
                mainMenu_->EditGame(gameName, mapInfo.mapName, mode);

            // Ready button
            if(IsGameOnGoing())
                ImGui::PushDisabled();

            if(ImGui::Button("Ready"))
                mainMenu_->ToggleReady();

            if(IsGameOnGoing())
                ImGui::PopDisabled();

            // Start Game Button
            bool disabled = IsGameInLobby() && (!isHost || !IsEveryoneReady());
            if(disabled)
                ImGui::PushDisabled();

            ImGui::BeginChild("Start Game Button", ImVec2(0, 0), false, 0);
                auto size = ImGui::GetContentRegionAvail();
                if(ImGui::Button("Start Game", size))
                {
                    if(IsGameInLobby())
                        mainMenu_->StartGame();
                    else if(IsGameOnGoing())
                        mainMenu_->LaunchGame();
                }
            ImGui::EndChild();

            if(disabled)
                ImGui::PopDisabled();

            ImGui::EndTable();
        }
    }
    // User clicked on the X button. Go back
    if(!show)
    {
        mainMenu_->LeaveGame();
    }

    ImGui::End();
}

bool Lobby::IsPlayerHost()
{
    for(auto playerInfo : mainMenu_->currentGame->playersInfo)
    {
        if(mainMenu_->user == playerInfo.playerName)
            return playerInfo.isHost;
    }

    return false;
}

bool Lobby::IsEveryoneReady()
{
    for(auto playerInfo : mainMenu_->currentGame->playersInfo)
    {
        if(!playerInfo.isHost && !playerInfo.isReady)
            return false;
    }

    return true;
}

bool Lobby::IsGameOnGoing()
{
    return mainMenu_->currentGame->game.state == "InGame";
}

bool Lobby::IsGameInLobby()
{
    return mainMenu_->currentGame->game.state == "InLobby";
}

void Lobby::SelectMap(MapInfo a)
{
    this->mapInfo = a;
    if(!a.supportedGamemodes.empty())
        this->mode = a.supportedGamemodes[0];
    
    mainMenu_->GetMapPicture(a.mapName);
}

// #### UPLOAD MAP #### \\

void UploadMap::OnEnter()
{
    auto& mapMgr = mainMenu_->GetMapMgr();
    auto localMaps = mapMgr.GetLocalMaps();
    for(auto& mapPath : localMaps)
        maps.push_back(mapPath.filename().string());

    password.resize(PASSWORD_MAX_SIZE);
}

void UploadMap::Update()
{
    auto displaySize = ImGui::GetIO().DisplaySize;

    // Centered
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});

    auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
    bool show = true;
    if(ImGui::Begin("Upload Map", &show, flags))
    {
        auto comboFlags = ImGuiComboFlags_None;
        if(ImGui::BeginCombo("Map", selectedMap.c_str(), comboFlags))
        {   
            for(auto map : maps)
            {
                bool selected = map == selectedMap;
                if(ImGui::Selectable(map.c_str(), selected))
                {
                    selectedMap = map;
                }
            }
            ImGui::EndCombo();
        }

        int textFlags = ImGuiInputTextFlags_CharsNoBlank;
        if(!showPass)
            textFlags |= ImGuiInputTextFlags_Password;
        ImGui::InputText("Password", password.data(), password.capacity(), textFlags);
        GUI::AddToolTip("This password will be necessary later, in case you want to update your map");

        ImGui::SameLine();
        if(ImGui::Button("Show"))
            showPass = !showPass;

        if(selectedMap.empty())
            ImGui::PushDisabled();

        if(ImGui::Button("Upload"))
        {
            mainMenu_->UploadMap(selectedMap, password.data());
        }

        if(selectedMap.empty())
            ImGui::PopDisabled();
    }
    // User clicked on the X button. Go back
    if(!show)
        mainMenu_->SetState(std::make_unique<MenuState::ServerBrowser>(mainMenu_));

    ImGui::End();
}