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
        int itFlags = mainMenu_->connecting ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;
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
            
            for(auto game : mainMenu_->gamesList)
            {
                ImGui::TableNextRow();
                auto gameId = std::string(game["id"]);
                auto name = std::string(game["name"]);

                ImGui::TableNextColumn();
                auto selectFlags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick;
                if(ImGui::Selectable(name.c_str(), false, selectFlags))
                {
                    if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_::ImGuiMouseButton_Left))
                        mainMenu_->JoinGame(gameId);
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

        if(ImGui::Button("Create Game"))
        {
            mainMenu_->SetState(std::make_unique<MenuState::CreateGame>(mainMenu_));
        }
        ImGui::SameLine();
        ImGui::Button("Connect");
        ImGui::SameLine();
        ImGui::Button("Filters");
        ImGui::SameLine();


        if(ImGui::Button("Update"))
        {
            mainMenu_->ListGames();
        }
    }
    // User clicked on the X button. Go back to login page
    if(!show)
        mainMenu_->SetState(std::make_unique<MenuState::Login>(mainMenu_));

    ImGui::End();
}

// #### CREATE GAME #### \\

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
        auto textFlags = ImGuiInputTextFlags_None;
        ImGui::InputText("Name", gameName, 32, textFlags);

        if(ImGui::Button("Create Game"))
        {
            mainMenu_->CreateGame(gameName);

            mainMenu_->SetState(std::make_unique<MenuState::Lobby>(mainMenu_));
        }
    }
    // User clicked on the X button. Go back
    if(!show)
        mainMenu_->SetState(std::make_unique<MenuState::ServerBrowser>(mainMenu_));

    ImGui::End();
}

// #### GAME #### \\

void Lobby::Update()
{
    auto displaySize = ImGui::GetIO().DisplaySize;
    
    // Size
    auto windowSize = ImVec2{displaySize.x * 0.7f, displaySize.y * 0.75f};
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

    // Centered
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});

    //TODO: Check if current game has value, should go back in that's not the case

    auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
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

                // Print player's info
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
            ImGui::Text("Map: Kobra");

            ImVec2 imgSize{0, layoutSize.y * 0.5f};
            //ImGui::Image()
            ImGui::Dummy(imgSize);
            ImGui::Text("Game Info");

            auto gitFlags = 0;
            ImVec2 gitSize{layoutSize.x - leftColSize, 0};
            if(ImGui::BeginTable("Game Info", 2, gitFlags, gitSize))
            {
                ImGui::TableNextColumn();
                ImGui::Text("%s", "Map");

                ImGui::TableNextColumn();
                ImGui::Text("%s", "Kobra");

                ImGui::EndTable();
            }

            // Chat window
            ImGui::TableNextColumn();
            auto inputLineSize = ImGui::CalcTextSize("Type ");

            ImGui::Text("Chat Window");
            auto chatFlags = ImGuiInputTextFlags_ReadOnly;
            auto height = ImGui::GetCursorPosY();
            float marginSize = 4.0f;
            ImVec2 chatSize{playerTableSize.x, (layoutSize.y - height) - inputLineSize.y * 2.0f - marginSize};
            ImGui::InputTextMultiline("##Chat Window", chat, 4096, chatSize, chatFlags);

            auto chatLineFlags = ImGuiInputTextFlags_EnterReturnsTrue;
            ImGui::Text("Type"); ImGui::SameLine(); 
            ImGui::PushItemWidth(playerTableSize.x - inputLineSize.x);
            ImGui::InputText("##Type", chatLine, 128, chatLineFlags);
            ImGui::PopItemWidth();

            ImGui::EndTable();
        }
    }
    // User clicked on the X button. Go back
    if(!show)
    {
        // LeaveGame();
        mainMenu_->SetState(std::make_unique<MenuState::ServerBrowser>(mainMenu_));
    }

    ImGui::End();
}