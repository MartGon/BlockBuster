#include <GameState/MainMenu/MainMenu.h>

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
    menuState_ = std::make_unique<MenuState::Login>(this);

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
    this->menuState_ = std::move(menuState);

    this->menuState_->OnEnter();
}