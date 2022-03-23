#include <GameState/InGame/InGameGUI.h>
#include <GameState/InGame/InGame.h>

#include <Client.h>
#include <VideoSettingsPopUp.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>

using namespace BlockBuster;

InGameGUI::InGameGUI(InGame& inGame) : inGame{&inGame}
{

}

void InGameGUI::Start()
{   
    // Load menu
    InitPopUps();

    // Init text
    std::filesystem::path fontPath = std::filesystem::path{RESOURCES_DIR} / "fonts/Pixel.ttf";
    pixelFont = GUI::TextFactory::Get()->LoadFont(fontPath);
    InitTexts();
    
    // Textures
    try{
        crosshair = GL::Texture::FromFolder(TEXTURES_DIR, "crosshairW.png");
        crosshair.Load();
    }
    catch(const std::runtime_error& e)
    {
        this->inGame->GetLogger()->LogError(e.what());
    }
    crosshairImg.SetTexture(&crosshair);
    crosshairImg.SetAnchorPoint(GUI::AnchorPoint::CENTER);
    crosshairImg.SetScale(glm::vec2{0.33f});
    crosshairImg.SetOffset(- crosshairImg.GetSize() / 2);
    crosshairImg.SetColor(glm::vec4{1.0f, 1.0f, 0.0f, 0.75f});
}

void InGameGUI::DrawGUI(GL::Shader& textShader)
{
    // Clear GUI buffer
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(inGame->client_->window_);
    ImGui::NewFrame();

    bool isOpen = true;
    if(inGame->connected)
    {
        DebugWindow();
        NetworkStatsWindow();
        RenderStatsWindow();
        puMgr.Update();
    }

    HUD();
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData(), guiVao.GetHandle());
    auto windowSize = inGame->client_->GetWindowSize();
    int scissor_box[4] = { 0, 0, windowSize.x, windowSize.y };
    ImGui_ImplOpenGL3_RestoreState(scissor_box);
}

void InGameGUI::OpenMenu()
{
    OpenMenu(InGameGUI::MENU);
}

void InGameGUI::OpenMenu(PopUpState state)
{
    puMgr.Open(state);

    SDL_SetWindowGrab(this->inGame->client_->window_, SDL_FALSE);
    SDL_SetRelativeMouseMode(SDL_FALSE);
}

void InGameGUI::InitPopUps()
{
    GUI::GenericPopUp menu;
    auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollWithMouse 
        | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize;
    auto onDrawMenu = [this](){
        auto size = ImGui::CalcTextSize("Video Settings ");
        size.y = 0;
        if(ImGui::Button("Resume", size))
            this->CloseMenu();

        if(ImGui::Button("Options", size))
            this->puMgr.Open(PopUpState::OPTIONS);

        if(ImGui::Button("Video Settings", size))
            this->puMgr.Open(PopUpState::VIDEO_SETTINGS);

        if(ImGui::Button("Exit Game", size))
            this->puMgr.Open(PopUpState::WARNING);
    };
    menu.SetOnDraw(onDrawMenu);
    menu.SetFlags(flags);
    menu.SetTitle("Menu");
    menu.SetCloseable(true);
    menu.SetOnClose([this](){
        this->CloseMenu();
    });
    puMgr.Set(MENU, std::make_unique<GUI::GenericPopUp>(menu));

    // Options Pop Up
    GUI::GenericPopUp options;
    auto onDrawOptions = [this](){
        ImGui::Text("Gameplay");
        ImGui::Separator();
        ImGui::SliderFloat("Sensitivity", &gameOptions.sensitivity, 0.1f, 5.0f, "%.2f");

        ImGui::Text("Sound");
        ImGui::Separator();
        ImGui::Checkbox("Sound enabled", &gameOptions.audioEnabled);
        ImGui::SliderInt("General",  &gameOptions.audioGeneral, 0, 100);

        auto winWidth = ImGui::GetWindowWidth();
        auto aW = ImGui::CalcTextSize("Apply").x + 8;
        auto aeW = ImGui::CalcTextSize("Apply and exit").x + 8;

        GUI::CenterSection(aW + aeW, winWidth);
        if(ImGui::Button("Apply"))
            this->inGame->ApplyGameOptions(this->gameOptions);
        ImGui::SameLine();
        if(ImGui::Button("Apply and exit"))
        {
            this->inGame->ApplyGameOptions(this->gameOptions);
            this->OpenMenu(PopUpState::MENU);
        }
    };
    options.SetOnOpen([this](){
        this->gameOptions = this->inGame->gameOptions;
    });
    options.SetOnClose([this](){
        this->OpenMenu(PopUpState::MENU);
    });
    options.SetTitle("Options");
    options.SetFlags(flags);
    options.SetOnDraw(onDrawOptions);
    options.SetCloseable(true);
    puMgr.Set(OPTIONS, std::make_unique<GUI::GenericPopUp>(options));

    // Video pop up
    auto videoSettings = std::make_unique<App::VideoSettingsPopUp>(*inGame->client_);
    videoSettings->SetOnClose([this](){
        this->OpenMenu(PopUpState::MENU);
    });
    puMgr.Set(VIDEO_SETTINGS, std::move(videoSettings));

    // Warning pop up
    GUI::GenericPopUp warning;
    warning.SetTitle("Exit Game");
    warning.SetFlags(flags);
    warning.SetOnDraw([this](){
        ImGui::Text("Are you sure?");
        auto size = ImGui::CalcTextSize("Yes ");
        size.y = 0;

        GUI::CenterSection(size.x * 2, ImGui::GetWindowWidth());
        if(ImGui::Button("Yes", size))
            this->inGame->client_->quit = true;
        ImGui::SameLine();
        if(ImGui::Button("No", size))
            this->CloseMenu();
    });
    puMgr.Set(WARNING, std::make_unique<GUI::GenericPopUp>(warning));
}

void InGameGUI::InitTexts()
{
    // Colors
    auto green = glm::vec4{0.1f, 0.9f, 0.1f, 1.0f};
    auto blue = glm::vec4{0.1f, 0.1f, 0.8f, 1.0f};
    auto lightBlue = glm::vec4{0.14f, 0.7f, 0.941f, 1.0f};
    auto red = glm::vec4{0.8f, 0.1f, 0.1f, 1.0f};
    auto golden = glm::vec4{0.7f, 0.7f, 0.1f, 1.0f};
    auto white = glm::vec4{1.0f};

    auto leftBorder = glm::ivec2{5, -5};

    // Health/Shield
    shieldIcon = pixelFont->CreateText();
    shieldIcon.SetText("U");
    shieldIcon.SetScale(2.0f);
    shieldIcon.SetColor(lightBlue);
    shieldIcon.SetAnchorPoint(GUI::AnchorPoint::UP_LEFT_CORNER);
    shieldIcon.SetOffset(glm::ivec2{0, -shieldIcon.GetSize().y} + leftBorder);

    armorText = pixelFont->CreateText();
    armorText.SetText("300/300");
    armorText.SetScale(2.0f);
    armorText.SetColor(lightBlue);
    armorText.SetAnchorPoint(GUI::AnchorPoint::DOWN_RIGHT_CORNER);
    armorText.SetParent(&shieldIcon);

    healthText = pixelFont->CreateText();
    healthText.SetText("030/100");
    healthText.SetScale(2);
    healthText.SetColor(green);
    healthText.SetAnchorPoint(GUI::AnchorPoint::DOWN_LEFT_CORNER);
    healthText.SetParent(&armorText);
    healthText.SetOffset(glm::ivec2{0, -healthText.GetSize().y} + leftBorder);

    healthIcon = pixelFont->CreateText();
    healthIcon.SetText("+");
    healthIcon.SetScale(3);
    healthIcon.SetColor(green);
    healthIcon.SetAnchorPoint(GUI::AnchorPoint::DOWN_LEFT_CORNER);
    healthIcon.SetParent(&healthText);
    healthIcon.SetOffset(glm::ivec2{-healthIcon.GetSize().x, -4});

    // Ammo
    ammoNumIcon = pixelFont->CreateText();
    ammoNumIcon.SetText("4");
    ammoNumIcon.SetScale(2.0f);
    ammoNumIcon.SetColor(golden);
    ammoNumIcon.SetAnchorPoint(GUI::AnchorPoint::UP_RIGHT_CORNER);
    ammoNumIcon.SetOffset(-ammoNumIcon.GetSize() + glm::ivec2{-5, -5});

    ammoText = pixelFont->CreateText();
    ammoText.SetText("llll");
    ammoText.SetScale(2.0f);
    ammoText.SetColor(golden);
    ammoText.SetParent(&ammoNumIcon);
    ammoText.SetAnchorPoint(GUI::AnchorPoint::DOWN_LEFT_CORNER);
    ammoText.SetOffset(glm::ivec2{-ammoText.GetSize().x, 0} + glm::ivec2{-5, 0});

    // Score
    midScoreText = pixelFont->CreateText();
    midScoreText.SetText(" - ");
    midScoreText.SetScale(2.f);
    midScoreText.SetColor(white);
    midScoreText.SetAnchorPoint(GUI::AnchorPoint::CENTER_UP);
    auto size = midScoreText.GetSize();
    midScoreText.SetOffset(glm::ivec2{-size.x / 2, -size.y * 8} + glm::ivec2{0, 0});

    leftScoreText = pixelFont->CreateText();
    leftScoreText.SetText("3");
    leftScoreText.SetScale(2.f);
    leftScoreText.SetColor(blue);
    leftScoreText.SetParent(&midScoreText);
    leftScoreText.SetAnchorPoint(GUI::AnchorPoint::DOWN_LEFT_CORNER);
    leftScoreText.SetOffset(glm::ivec2{-leftScoreText.GetSize().x, -5});

    rightScoreText = pixelFont->CreateText();
    rightScoreText.SetText("6");
    rightScoreText.SetScale(2.f);
    rightScoreText.SetColor(red);
    rightScoreText.SetParent(&midScoreText);
    rightScoreText.SetAnchorPoint(GUI::AnchorPoint::DOWN_RIGHT_CORNER);
    rightScoreText.SetOffset(glm::ivec2{0, -5});
}

void InGameGUI::HUD()
{
    auto winSize = inGame->client_->GetWindowSize();

    UpdateHealth();
    UpdateArmor();
    UpdateAmmo();

    armorText.Draw(inGame->textShader, winSize);
    shieldIcon.Draw(inGame->textShader, winSize);
    healthText.Draw(inGame->textShader, winSize);
    healthIcon.Draw(inGame->textShader, winSize);
    ammoNumIcon.Draw(inGame->textShader, winSize);
    ammoText.Draw(inGame->textShader, winSize);

    midScoreText.Draw(inGame->textShader, winSize);
    leftScoreText.Draw(inGame->textShader, winSize);
    rightScoreText.Draw(inGame->textShader, winSize);

    crosshairImg.Draw(inGame->imgShader, winSize);
}

void InGameGUI::UpdateHealth()
{
    auto& player = inGame->GetLocalPlayer();
    auto health = player.health;

    auto healthStr = GetBoundedValue(glm::ceil(health), player.MAX_HEALTH);
    healthText.SetText(healthStr);
}

void InGameGUI::UpdateArmor()
{
    auto& player = inGame->GetLocalPlayer();
    auto armor = player.shield;

    auto armorStr = GetBoundedValue(glm::ceil(armor), player.MAX_SHIELD);
    armorText.SetText(armorStr);
    ammoText.SetOffset(glm::ivec2{-ammoText.GetSize().x, 0} + glm::ivec2{-5, 0});
}

std::string InGameGUI::GetBoundedValue(int val, int max)
{
    auto str = std::to_string(val);
    auto targetSize = glm::floor(std::log10(max)) + 1;

    auto diff = targetSize - str.size();
    std::string prefix;
    prefix.resize(diff, '0');

    str = prefix + str + "/" + std::to_string(max);

    return str;
}

void InGameGUI::UpdateAmmo()
{
    auto& player = inGame->GetLocalPlayer();

    // TODO: Update for overheat weapon
    auto ammo = player.weapon.ammoState.magazine;
    
    ammoNumIcon.SetText(std::to_string(ammo));

    std::string ammoStr;
    ammoStr.resize(ammo, 'l');
    ammoText.SetText(ammoStr);
}

void InGameGUI::UpdateScore()
{
    
}

void InGameGUI::CloseMenu()
{
    if(this->inGame->camController_.GetMode() != App::Client::CameraMode::EDITOR)
    {
        SDL_SetWindowGrab(this->inGame->client_->window_, SDL_TRUE);
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }

    puMgr.Close();
}

bool InGameGUI::IsMenuOpen()
{
    return puMgr.IsOpen();
}

void InGameGUI::DebugWindow()
{
    if(ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if(ImGui::CollapsingHeader("Transform"))
        {
            auto t = inGame->playerTable[inGame->playerId].GetTransform();
            inGame->GetLogger()->LogInfo("Player pos" + glm::to_string(t.position));
            modelOffset = t.position;
            modelScale =  t.scale;
            modelRot =  t.rotation;

            /*
            if(ImGui::InputInt("ID", (int*)&playerId))
            {
                if(auto sm = playerAvatar.armsModel->GetSubModel(modelId))
                {
                    auto t = playerTable[playerId].GetTransform();
                    GetLogger()->LogInfo("Player pos" + glm::to_string(t.position));
                    modelOffset = t.position;
                    modelScale =  t.scale;
                    modelRot =  t.rotation;
                }
            }*/
            ImGui::SliderFloat3("Offset", &modelOffset.x, -sliderPrecision, sliderPrecision);
            ImGui::SliderFloat3("Scale", &modelScale.x, -sliderPrecision, sliderPrecision);
            ImGui::SliderFloat3("Rotation", &modelRot.x, -sliderPrecision, sliderPrecision);
            ImGui::InputFloat("Precision", &sliderPrecision);
            if(ImGui::Button("Apply"))
            {
                // Edit player model
                if(auto sm = inGame->playerAvatar.armsModel->GetSubModel(modelId))
                {
                    inGame->playerAvatar.aTransform.position = modelOffset;
                    inGame->playerAvatar.aTransform.scale = modelScale;
                    inGame->playerAvatar.aTransform.rotation = modelRot;
                }
            }
        }

        if(ImGui::CollapsingHeader("Animations"))
        {
            if(ImGui::Button("Shoot"))
            {
                inGame->fpsAvatar.PlayShootAnimation();
                for(auto& [playerId, playerState] : inGame->playerModelStateTable)
                {
                    playerState.shootPlayer.SetClip(inGame->playerAvatar.GetReloadAnim());
                    playerState.shootPlayer.Reset();
                    playerState.shootPlayer.Play();
                    break;
                }
            }
        }

        if(ImGui::CollapsingHeader("Audio"))
        {
            ImGui::Text("Audio");
            ImGui::Separator();

            auto params = inGame->audioMgr->GetSourceParams(0);
            if(ImGui::SliderFloat("Gain", &params->gain, 0.0f, 1.0f))
                inGame->audioMgr->SetSourceParams(0, *params);
            if(ImGui::SliderFloat3("Pos", &params->pos.x, 0.0f, 1000.0f))
                inGame->audioMgr->SetSourceParams(0, *params);

            if(ImGui::Button("Play audio"))
            {
                inGame->audioMgr->PlaySource(0);
            }
        }

        if(ImGui::CollapsingHeader("HUD"))
        {
            auto winSize = inGame->client_->GetWindowSize();

            auto size = healthText.GetSize();
            auto tPos = healthText.GetPos(winSize);
            ImGui::SliderInt2("Health Text pos", &wtPos.x, -winSize.x / 2, winSize.x / 2);
            ImGui::SliderFloat("Health Scale", &tScale, 0.1f, 5.0f);
            ImGui::SliderInt2("Text Size", &size.x, 0, winSize.x);
            ImGui::SliderInt2("Text Pos", &tPos.x, -winSize.x / 2, winSize.x / 2);
        }
    }
    ImGui::End();
}

void InGameGUI::NetworkStatsWindow()
{
    if(ImGui::Begin("Network Stats"))
    {
        auto info = inGame->host.GetPeerInfo(inGame->serverId);

        ImGui::Text("Config");
        ImGui::Separator();
        ImGui::Text("Server tick rate: %.2f ms", inGame->serverTickRate.count());

        ImGui::Text("Latency");
        ImGui::Separator();
        ImGui::Text("Ping:");ImGui::SameLine();ImGui::Text("%i ms", info.roundTripTimeMs);
        ImGui::Text("Ping variance: %i", info.roundTripTimeVariance);

        ImGui::Text("Packets");
        ImGui::Separator();
        ImGui::Text("Packets sent: %i", info.packetsSent);
        ImGui::Text("Packets ack: %i", info.packetsAck);
        ImGui::Text("Packets lost: %i", info.packetsLost);
        ImGui::Text("Packet loss: %i", info.packetLoss);

        ImGui::Text("Bandwidth");
        ImGui::Separator();
        ImGui::Text("In: %i B/s", info.incomingBandwidth);
        ImGui::Text("Out: %i B/s", info.outgoingBandwidth);
    }
    ImGui::End();
}

void InGameGUI::RenderStatsWindow()
{
    if(ImGui::Begin("Rendering Stats"))
    {
        ImGui::Text("Latency");
        ImGui::Separator();
        uint64_t frameIntervalMs = inGame->deltaTime.count() * 1e3;
        ImGui::Text("Frame interval (delta time):");ImGui::SameLine();ImGui::Text("%lu ms", frameIntervalMs);
        uint64_t fps = 1.0 / inGame->deltaTime.count();
        ImGui::Text("FPS: %lu", fps);

        ImGui::InputDouble("Max FPS", &maxFPS, 1.0, 5.0, "%.0f");
    }
    ImGui::End();
}
