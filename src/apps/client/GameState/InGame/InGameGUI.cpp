#include <GameState/InGame/InGameGUI.h>
#include <GameState/InGame/InGame.h>

#include <Client.h>
#include <VideoSettingsPopUp.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <sstream>

#include <util/Container.h>

#include <Event.h>


using namespace BlockBuster;

InGameGUI::InGameGUI(InGame& inGame) : inGame{&inGame}
{

}

void InGameGUI::Start()
{   
    // Load menu
    InitPopUps();

    // Init text
    std::filesystem::path fontPath = inGame->client_->resourcesDir / "fonts/Pixel.ttf";
    pixelFont = GUI::TextFactory::Get()->LoadFont(fontPath);
    InitTexts();
    
    // Textures
    auto& textureMgr = inGame->renderMgr.GetTextureMgr();
    hitmarker = textureMgr.LoadFromDefaultFolder("hitmarker.png");
    grenadeTexId = textureMgr.LoadFromDefaultFolder("grenade2.png", true);
    dmgArrowId = textureMgr.LoadFromDefaultFolder("arrow.png");
    glm::u8vec4 white{255, 255, 255, 255};
    dmgTexture = textureMgr.LoadRaw(&white.x, glm::ivec2{1, 1}, GL_RGBA);

        // Crosshairs
    for(int i = Entity::WeaponTypeID::ASSAULT_RIFLE; i < Entity::WeaponTypeID::CHEAT_SMG; i++)
    {
        auto filename = "crosshair-" + std::to_string(i) + "-w.png";
        auto texId = textureMgr.LoadFromDefaultFolder(filename);
        crosshairTextures.Add(i, texId);
    }

    // Images
    auto winSize = inGame->client_->GetWindowSize();

    using namespace Entity;
    wepIcon.SetTexture(inGame->modelMgr.GetWepIconTex(WeaponTypeID::ASSAULT_RIFLE));
    wepIcon.SetAnchorPoint(GUI::AnchorPoint::DOWN_RIGHT_CORNER);
    wepIcon.SetParent(&ammoNumIcon);
    wepIcon.SetSize(glm::vec2{220, 70} * 0.75f);
    auto iconSize = wepIcon.GetSize();
    wepIcon.SetOffset(-iconSize);
    wepIcon.SetIsVisible(true);

    altWepIcon.SetTexture(inGame->modelMgr.GetWepIconTex(WeaponTypeID::ASSAULT_RIFLE));
    altWepIcon.SetAnchorPoint(GUI::AnchorPoint::DOWN_RIGHT_CORNER);
    altWepIcon.SetParent(&wepIcon);
    altWepIcon.SetSize(glm::vec2{220, 70} * 0.33f);
    iconSize = altWepIcon.GetSize();
    altWepIcon.SetOffset(-iconSize);
    altWepIcon.SetIsVisible(false);

    actionImg.SetTexture(textureMgr.GetTexture(grenadeTexId));
    actionImg.SetParent(&altWepIcon);
    actionImg.SetAnchorPoint(GUI::AnchorPoint::DOWN_RIGHT_CORNER);
    actionImg.SetScale(glm::vec2{0.5f});
    iconSize = actionImg.GetSize();
    actionImg.SetOffset(-iconSize);
    actionImg.SetIsVisible(false);

    grenadeIcon.SetTexture(textureMgr.GetTexture(grenadeTexId));
    grenadeIcon.SetAnchorPoint(GUI::AnchorPoint::DOWN_LEFT_CORNER);
    grenadeIcon.SetParent(&grenadeNumText);
    grenadeIcon.SetSize(glm::ivec2{35, 35});
    iconSize = grenadeIcon.GetSize();
    grenadeIcon.SetOffset(glm::ivec2{-iconSize.x, -7});

    crosshairImg.SetTexture(textureMgr.GetTexture(crosshairTextures.Get(Entity::WeaponTypeID::ASSAULT_RIFLE).value()));
    crosshairImg.SetAnchorPoint(GUI::AnchorPoint::CENTER);
    crosshairImg.SetSize(glm::ivec2{75});
    crosshairImg.SetOffset(- crosshairImg.GetSize() / 2);
    crosshairImg.SetColor(glm::vec4{1.0f, 1.0f, 0.0f, 0.75f});

    hitmarkerImg.SetTexture(textureMgr.GetTexture(hitmarker));
    hitmarkerImg.SetAnchorPoint(GUI::AnchorPoint::CENTER);
    hitmarkerImg.SetSize(crosshairImg.GetSize());
    hitmarkerImg.SetOffset(-hitmarkerImg.GetSize() / 2);
    hitmarkerImg.SetColor(glm::vec4{1.0f, 1.0f, 1.0f, 0.5f});

    flagIconImg.SetTexture(inGame->modelMgr.GetIconTex(Game::Models::FLAG_ICON_ID));
    flagIconImg.SetAnchorPoint(GUI::AnchorPoint::DOWN_LEFT_CORNER);
    flagIconImg.SetParent(&logText);
    flagIconImg.SetSize(glm::ivec2{64, 64});
    flagIconImg.SetOffset(glm::ivec2{0, -flagIconImg.GetSize().y} + glm::ivec2{0, -5});
    flagIconImg.SetIsVisible(false);

    dmgEffectImg.SetTexture(textureMgr.GetTexture(dmgTexture));
    dmgEffectImg.SetAnchorPoint(GUI::AnchorPoint::DOWN_LEFT_CORNER);

    dmgArrowImg.SetTexture(textureMgr.GetTexture(dmgArrowId));
    dmgArrowImg.SetAnchorPoint(GUI::AnchorPoint::CENTER);
    dmgArrowImg.SetSize(glm::ivec2{26, 64});
    dmgArrowImg.SetOffset(glm::ivec2{winSize.x * 0.1f, -32});

    // Animations
    InitAnimations();
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
    #ifdef _DEBUG
        DebugWindow();
        NetworkStatsWindow();
        RenderStatsWindow();
    #endif
        puMgr.Update();

        bool show = showScoreboard || inGame->match.GetState() == Match::StateType::ENDED;
        if(show && !IsMenuOpen())
            ScoreboardWindow();

        HUD();
    }
    
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
    inGame->client_->SetMouseGrab(false);
}

void InGameGUI::CloseMenu()
{
    if(this->inGame->camController_.GetMode() != App::Client::CameraMode::EDITOR)
    {
        inGame->client_->SetMouseGrab(true);
    }

    puMgr.Close();
}

bool InGameGUI::IsMenuOpen()
{
    return puMgr.IsOpen();
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
        ImGui::SliderInt("Announcer",  &gameOptions.audioAnnouncer, 0, 100);

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
            this->inGame->Exit();
        ImGui::SameLine();
        if(ImGui::Button("No", size))
            this->CloseMenu();
    });
    puMgr.Set(WARNING, std::make_unique<GUI::GenericPopUp>(warning));
}

void InGameGUI::InitTexts()
{
    auto winSize = inGame->client_->GetWindowSize();

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

    grenadeNumText = pixelFont->CreateText();
    grenadeNumText.SetText("0");
    grenadeNumText.SetScale(1.0f);
    grenadeNumText.SetColor(glm::vec4{0.173f, 0.655f, 0.78f, 0.75f});
    grenadeNumText.SetParent(&wepIcon);
    grenadeNumText.SetAnchorPoint(GUI::AnchorPoint::UP_LEFT_CORNER);
    auto gSize = grenadeNumText.GetSize();
    grenadeNumText.SetOffset(-gSize + glm::ivec2{-20, -14});

    actionText = pixelFont->CreateText();
    actionText.SetText("Press E to use ");
    actionText.SetScale(1.0f);
    actionText.SetColor(glm::vec4{0.173f, 0.655f, 0.78f, 0.75f});
    actionText.SetParent(&actionImg);
    actionText.SetAnchorPoint(GUI::AnchorPoint::CENTER_LEFT);
    auto asize = actionText.GetSize();
    actionText.SetOffset(glm::ivec2{-asize.x, -asize.y / 2});
    actionText.SetIsVisible(false);

    // Score
    midScoreText = pixelFont->CreateText();
    midScoreText.SetText(" - ");
    midScoreText.SetScale(2.f);
    midScoreText.SetColor(white);
    midScoreText.SetAnchorPoint(GUI::AnchorPoint::CENTER_UP);
    auto size = midScoreText.GetSize();
    midScoreText.SetOffset(glm::ivec2{-size.x / 2, -size.y * 8} + glm::ivec2{0, 0});
    midScoreText.SetIsVisible(false);

    leftScoreText = pixelFont->CreateText();
    leftScoreText.SetText("0");
    leftScoreText.SetScale(2.f);
    leftScoreText.SetColor(blue);
    leftScoreText.SetParent(&midScoreText);
    leftScoreText.SetAnchorPoint(GUI::AnchorPoint::DOWN_LEFT_CORNER);
    leftScoreText.SetOffset(glm::ivec2{-leftScoreText.GetSize().x, -5});
    leftScoreText.SetIsVisible(false);

    rightScoreText = pixelFont->CreateText();
    rightScoreText.SetText("0");
    rightScoreText.SetScale(2.f);
    rightScoreText.SetColor(red);
    rightScoreText.SetParent(&midScoreText);
    rightScoreText.SetAnchorPoint(GUI::AnchorPoint::DOWN_RIGHT_CORNER);
    rightScoreText.SetOffset(glm::ivec2{0, -5});
    rightScoreText.SetIsVisible(false);

    gameTimeText = pixelFont->CreateText();
    gameTimeText.SetText("10:23");
    gameTimeText.SetScale(1.f);
    gameTimeText.SetColor(white);
    gameTimeText.SetParent(&midScoreText);
    gameTimeText.SetAnchorPoint(GUI::AnchorPoint::CENTER_DOWN);
    size = gameTimeText.GetSize();
    gameTimeText.SetOffset(glm::ivec2{-size.x / 2, -size.y} + glm::ivec2{0, -5});
    gameTimeText.SetIsVisible(false);

    winnerText = pixelFont->CreateText();
    winnerText.SetText("RED TEAM");
    winnerText.SetAnchorPoint(GUI::AnchorPoint::CENTER);
    winnerText.SetColor(red);
    winnerText.SetScale(3.0f);
    winnerText.SetOffset(-winnerText.GetSize() / 2);
    winnerText.SetIsVisible(false);

    winnerAnnoucerText = pixelFont->CreateText();
    winnerAnnoucerText.SetText("THE WINNER IS");
    winnerAnnoucerText.SetScale(1.5f);
    winnerAnnoucerText.SetColor(white);
    winnerAnnoucerText.SetParent(&winnerText);
    winnerAnnoucerText.SetAnchorPoint(GUI::AnchorPoint::CENTER_UP);
    size = winnerAnnoucerText.GetSize();
    winnerAnnoucerText.SetOffset(glm::ivec2{-size.x / 2, 0} + glm::ivec2{0, 20});
    winnerAnnoucerText.SetIsVisible(false);

    // Logs
    killText = pixelFont->CreateText();
    killText.SetText("YOU DIED");
    killText.SetColor(red);
    killText.SetAnchorPoint(GUI::AnchorPoint::CENTER);
    killText.SetScale(2.0f);
    killText.SetOffset(-killText.GetSize() / 2);
    killText.SetIsVisible(false);

    respawnTimeText = pixelFont->CreateText();
    respawnTimeText.SetText("You'll respawn in 5 seconds");
    respawnTimeText.SetColor(white);
    respawnTimeText.SetParent(&killText);
    respawnTimeText.SetAnchorPoint(GUI::AnchorPoint::CENTER_DOWN);
    respawnTimeText.SetScale(1.5f);
    respawnTimeText.SetOffset(glm::ivec2{-respawnTimeText.GetSize().x / 2, 0});
    respawnTimeText.SetIsVisible(false);

    countdownText = pixelFont->CreateText();
    countdownText.SetText("15");
    countdownText.SetColor(white);
    countdownText.SetAnchorPoint(GUI::AnchorPoint::CENTER);
    countdownText.SetScale(3.0f);
    countdownText.SetIsVisible(false);

    capturingText = pixelFont->CreateText();
    capturingText.SetText("CAPTURING");
    capturingText.SetColor(white);
    capturingText.SetParent(&gameTimeText);
    capturingText.SetAnchorPoint(GUI::AnchorPoint::CENTER_DOWN);
    capturingText.SetScale(1.5f);
    size = capturingText.GetSize();
    //capturingText.SetOffset(glm::ivec2{-size.x / 2, -size.y} + glm::ivec2{6, -10});

    captCountdownText = pixelFont->CreateText();
    captCountdownText.SetText("0");
    captCountdownText.SetColor(white);
    captCountdownText.SetParent(&capturingText);
    captCountdownText.SetAnchorPoint(GUI::AnchorPoint::CENTER_DOWN);
    captCountdownText.SetScale(2.0f);
    EnableCapturingText(false);

    logText = pixelFont->CreateText();
    logText.SetText("You killed Defu#4154");
    logText.SetParent(&healthIcon);
    logText.SetColor(white);
    logText.SetAnchorPoint(GUI::AnchorPoint::DOWN_LEFT_CORNER);
    logText.SetScale(0.75f);
    logText.SetOffset(glm::ivec2{0, -logText.GetSize().y} + glm::ivec2{0, -10});
    logText.SetIsVisible(false);

}

void InGameGUI::InitAnimations()
{
    // Hit marker anim
    Animation::Sample s1{
        {},
        {{"show", true}}
    };
    Animation::KeyFrame f1{s1, 0};    
    Animation::Sample s2{
        {},
        {{"show", false}}
    };
    Animation::KeyFrame f2{s2, 30};
    hitmarkerAnim.keyFrames = {f1, f2};
    hitMarkerPlayer.SetClip(&hitmarkerAnim);
    hitMarkerPlayer.SetTargetBool("show", &showHitmarker);

    // Dmg anim
    Animation::Sample ds1{
        {{"alpha", 0.65f}},
        {}
    };
    Animation::KeyFrame df1{ds1, 0};
    Animation::KeyFrame df2{ds1, 10};
    Animation::Sample ds2{
        {{"alpha", 0.0f}},
        {}
    };
    Animation::KeyFrame df3{ds2, 20};
    dmgAnim.keyFrames = {df1, df2, df3};
    dmgAnimationPlayer.SetClip(&dmgAnim);
    dmgAnimationPlayer.SetTargetFloat("alpha", &dmgAlpha);

    // Log anim
     Animation::Sample ls1{
        {{"alpha", 1.f}},
        {}
    };
    Animation::KeyFrame lf1{ls1, 0};
    Animation::Sample ls2{
        {{"alpha", 0.0f}},
        {}
    };
    Animation::KeyFrame lf2{ls2, 180};
    logAnim.keyFrames = {lf1, lf2};
    logAnimPlayer.SetClip(&logAnim);
    logAnimPlayer.SetTargetFloat("alpha", &logAlpha);

    // Dmg arrow
    dmgArrowAnimationPlayer.SetClip(&logAnim);
    dmgArrowAnimationPlayer.SetTargetFloat("alpha", &dmgArrowAlpha);
    dmgArrowAnimationPlayer.SetSpeed(2.0f);
}

void InGameGUI::HUD()
{
    glDisable(GL_DEPTH_TEST);
    auto winSize = inGame->client_->GetWindowSize();
    auto& textureMgr = inGame->renderMgr.GetTextureMgr();
    auto& player = inGame->GetLocalPlayer();

    UpdateHealth();
    UpdateArmor();
    UpdateAmmo();
    UpdateScore();
    UpdateRespawnText();
    UpdateCountdownText();
    UpdateGameTimeText();

    // HUD
    armorText.Draw(inGame->textShader, winSize);
    shieldIcon.Draw(inGame->textShader, winSize);
    healthText.Draw(inGame->textShader, winSize);
    healthIcon.Draw(inGame->textShader, winSize);
    ammoNumIcon.Draw(inGame->textShader, winSize);
    ammoText.Draw(inGame->textShader, winSize);

        // Wep Icon
    auto wepTypeId = player.GetCurrentWeapon().weaponTypeId;
    wepIcon.SetTexture(inGame->modelMgr.GetWepIconTex(wepTypeId));
    wepIcon.Draw(inGame->imgShader, winSize);

        // Alt wep Icon
    auto nextWepTypeId = player.weapons[player.GetNextWeaponId()].weaponTypeId;
    if(nextWepTypeId != Entity::WeaponTypeID::NONE)
    {
        altWepIcon.SetTexture(inGame->modelMgr.GetWepIconTex(nextWepTypeId));
        altWepIcon.SetIsVisible(true);
    }
    else
        altWepIcon.SetIsVisible(false);
    altWepIcon.Draw(inGame->imgShader, winSize);

        // Grenade icon/text
    grenadeNumText.SetText(std::to_string(player.grenades));
    grenadeNumText.Draw(inGame->textShader, winSize);
    grenadeIcon.Draw(inGame->imgShader, winSize);

        // Action text
    actionImg.Draw(inGame->imgShader, winSize);
    actionText.Draw(inGame->textShader, winSize);

    // Score
    midScoreText.Draw(inGame->textShader, winSize);
    leftScoreText.Draw(inGame->textShader, winSize);
    rightScoreText.Draw(inGame->textShader, winSize);
    gameTimeText.Draw(inGame->textShader, winSize);

    winnerAnnoucerText.Draw(inGame->textShader, winSize);
    winnerText.Draw(inGame->textShader, winSize);

    // Hit marker
    hitmarkerImg.SetIsVisible(showHitmarker);
    hitmarkerImg.Draw(inGame->imgShader, winSize);
    hitMarkerPlayer.Update(inGame->deltaTime);

    // Croshair
    if(auto texId = crosshairTextures.Get(wepTypeId))
    {
        crosshairImg.SetTexture(textureMgr.GetTexture(texId.value()));
        crosshairImg.SetSize(glm::ivec2{75});
    }
    crosshairImg.Draw(inGame->imgShader, winSize);

    // You've been killed
    killText.Draw(inGame->textShader, winSize);
    respawnTimeText.Draw(inGame->textShader, winSize);

    // Countdown
    auto ccSize = countdownText.GetSize();
    countdownText.SetOffset(glm::ivec2{-ccSize.x / 2, ccSize.y});
    countdownText.Draw(inGame->textShader, winSize);

    // Dmg animation
    dmgAnimationPlayer.Update(inGame->deltaTime);
    dmgEffectImg.SetScale(winSize);
    const auto color = glm::vec4{effectColor[effectType], dmgAlpha};
    dmgEffectImg.SetColor(color);
    dmgEffectImg.Draw(inGame->imgShader, winSize);

        // Dmg arrow
    dmgArrowAnimationPlayer.Update(inGame->deltaTime);
    auto view = inGame->camera_.GetViewMat();
    glm::vec3 dmgOrigin = view * glm::vec4{inGame->lastDmgOrigin, 1.0f};
    auto dmgOrigin2d = glm::vec2{dmgOrigin.x, dmgOrigin.z};
    
    auto dir = glm::normalize(dmgOrigin2d); dir.y = -dir.y;
    auto pos = dir * (glm::vec2{winSize} * 0.1f);
    glm::vec2 arrowSize = dmgArrowImg.GetSize();
    auto angle = glm::atan(dir.y, dir.x);
    dmgArrowImg.SetRot(angle);
    dmgArrowImg.SetOffset(pos - arrowSize / 2.0f);
    auto arrowColor = glm::vec4{1.0f, 0.0f, 0.0f, dmgArrowAlpha};
    dmgArrowImg.SetColor(arrowColor);
    dmgArrowImg.Draw(inGame->imgShader, winSize);

    // Capturing
    auto size = capturingText.GetSize();
    capturingText.SetOffset(glm::ivec2{-size.x / 2, -size.y} + glm::ivec2{0, -10});
    capturingText.Draw(inGame->textShader, winSize);
    size = captCountdownText.GetSize();
    captCountdownText.SetOffset(glm::ivec2{-size.x / 2, 0} + glm::ivec2{0, -15});
    captCountdownText.Draw(inGame->textShader, winSize);

    // Log
    logAnimPlayer.Update(inGame->deltaTime);
    const auto white = glm::vec4{1.0f, 1.0f, 1.0f, logAlpha};
    logText.SetColor(white);
    logText.Draw(inGame->textShader, winSize);

    // Flag Icon
    auto teamColor = GetOppositeColor(inGame->GetLocalPlayer().teamId);
    flagIconImg.SetColor(teamColor);
    flagIconImg.Draw(inGame->imgShader, winSize);

    glEnable(GL_DEPTH_TEST);
}

void InGameGUI::UpdateHealth()
{
    auto& player = inGame->GetLocalPlayer();
    auto health = player.health;

    auto healthStr = GetBoundedValue(glm::ceil(health.hp), player.MAX_HEALTH);
    healthText.SetText(healthStr);
}

void InGameGUI::UpdateArmor()
{
    auto& player = inGame->GetLocalPlayer();
    auto armor = player.health.shield;

    auto armorStr = GetBoundedValue(glm::ceil(armor), player.MAX_SHIELD);
    armorText.SetText(armorStr);
    ammoText.SetOffset(glm::ivec2{-ammoText.GetSize().x, 0} + glm::ivec2{-5, 0});
}

std::string InGameGUI::GetBoundedValue(int val, int max)
{
    auto targetSize = glm::floor(std::log10(max)) + 1;
    auto str = LeadingZeros(val, targetSize);

    str = str + "/" + std::to_string(max);

    return str;
}

std::string InGameGUI::LeadingZeros(int val, int size)
{
    std::string str = std::to_string(val);
    auto diff = size - str.size();
    std::string prefix;
    prefix.resize(diff, '0');

    str = prefix + str;
    return str;
}

void InGameGUI::UpdateAmmo()
{
    auto& player = inGame->GetLocalPlayer();
    constexpr const int MAX_DISPLAY = 30;

    auto& weapon = Entity::WeaponMgr::weaponTypes.at(player.GetCurrentWeapon().weaponTypeId);

    int ammoNum = 0;
    int size = 0;
    if(weapon.ammoType == Entity::AmmoType::AMMO)
    {
        ammoNum = player.GetCurrentWeapon().ammoState.magazine;
        size = ammoNum;
    }
    else if(weapon.ammoType == Entity::AmmoType::OVERHEAT)
    {
        ammoNum = std::ceil(player.GetCurrentWeapon().ammoState.overheat);
        constexpr float rate = MAX_DISPLAY / Entity::MAX_OVERHEAT;
        size = ammoNum * rate;
    }
    
    ammoNumIcon.SetText(std::to_string(ammoNum));
    ammoNumIcon.SetOffset(-ammoNumIcon.GetSize() + glm::ivec2{-5, -5});

    std::string ammoStr;
    size = std::min(size, MAX_DISPLAY);
    ammoStr.resize(size, 'l');
    ammoText.SetText(ammoStr);
}

void InGameGUI::UpdateScore()
{
    auto mode =inGame->match.GetGameMode();
    auto scoreBoard = mode->GetScoreboard();
    if(mode->GetType() == GameMode::FREE_FOR_ALL)
    {
        auto playerScore = scoreBoard.GetPlayerScore(inGame->playerId);
        if(!playerScore)
            return;
        leftScoreText.SetText(std::to_string(playerScore->score));
        leftScoreText.SetColor(inGame->ffaColors[inGame->playerId]);        

        auto scores = scoreBoard.GetTeamScores();
        std::sort(scores.begin(), scores.end(), [](auto a, auto b){
            return a.score > b.score;
        });
        auto first = scores[0];
        if(first.teamId == inGame->playerId && scores.size() > 1)
            first = scores[1];
        rightScoreText.SetText(std::to_string(first.score));
        rightScoreText.SetColor(inGame->ffaColors[first.teamId]);
    }
    else
    {
        auto blueTeamScore = scoreBoard.GetTeamScore(TeamID::BLUE_TEAM_ID);
        auto redTeamScore = scoreBoard.GetTeamScore(TeamID::RED_TEAM_ID);

        if(blueTeamScore)
            leftScoreText.SetText(std::to_string(blueTeamScore->score));
        if(redTeamScore)
            rightScoreText.SetText(std::to_string(redTeamScore->score));
    }

    leftScoreText.SetOffset(glm::ivec2{-leftScoreText.GetSize().x, -5});
}

void InGameGUI::UpdateRespawnText()
{
    auto respawnTime = inGame->respawnTimer.GetTimeLeft();
    int seconds = std::ceil(respawnTime.count());
    std::string text = "Respawning in " + std::to_string(seconds) + " seconds";
    respawnTimeText.SetText(text);
    respawnTimeText.SetOffset(glm::ivec2{-respawnTimeText.GetSize().x / 2, 0.0f});
}

void InGameGUI::UpdateCountdownText()
{
    if(inGame->match.GetState() == Match::StateType::STARTING)
    {
        auto countdownTime = inGame->match.GetTimeLeft();
        int seconds = std::ceil(countdownTime.count());
        std::string text = std::to_string(seconds);
        countdownText.SetText(text);
    }
}

void InGameGUI::UpdateGameTimeText()
{
    auto countdownTime = inGame->match.GetTimeLeft();
    int secs = countdownTime.count();
    int minutes = secs / 60;
    int offsetSecs = secs % 60;

    std::string minStr = LeadingZeros(minutes, 2);
    std::string secsStr = LeadingZeros(offsetSecs, 2);
    
    std::string text = minStr + ':' + secsStr;
    gameTimeText.SetText(text);
    auto size = gameTimeText.GetSize();
    gameTimeText.SetOffset(glm::ivec2{-size.x / 2, -size.y} + glm::ivec2{0, -5});
}

void InGameGUI::PlayHitMarkerAnim(HitMarkerType type)
{
    if(type == HitMarkerType::DMG)
    {  
        const auto white = glm::vec4{1.0f, 1.0f, 1.0f, 0.5f};
        hitmarkerImg.SetColor(white);
    }
    else if(type == HitMarkerType::KILL)
    {
        const auto red = glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};
        hitmarkerImg.SetColor(red);
    }

    hitMarkerPlayer.Restart();
}

void InGameGUI::PlayScreenEffect(ScreenEffect effect)
{
    effectType = effect;
    dmgAnimationPlayer.Restart();
    if(effectType == ScreenEffect::SCREEN_EFFECT_DMG)
    {
        dmgArrowAnimationPlayer.Restart();
    }
}

void InGameGUI::EnableActionText(Entity::GameObject& go)
{
    using namespace Entity;
    auto& textureMgr = inGame->renderMgr.GetTextureMgr();
    if(go.type == GameObject::Type::GRENADES)
    {
        actionImg.SetTexture(textureMgr.GetTexture(grenadeTexId));
        actionImg.SetScale(glm::vec2{0.5f, 0.5f});
    }
    else if(go.type == GameObject::Type::HEALTHPACK)
    {
        actionImg.SetTexture(inGame->modelMgr.GetIconTex(Game::Models::RED_CROSS_ICON_ID));
        actionImg.SetSize(glm::ivec2{35, 35});
    }
    else if(go.type == GameObject::Type::WEAPON_CRATE)
    {
        auto wepId = static_cast<Entity::WeaponTypeID>(std::get<int>(go.properties["Weapon ID"].value));
        actionImg.SetTexture(inGame->modelMgr.GetWepIconTex(wepId));
        actionImg.SetScale(glm::vec2{0.33f});
    }
    auto size = actionImg.GetSize();
    actionImg.SetOffset(-size);

    actionImg.SetIsVisible(true);
    actionText.SetIsVisible(true);
}

void InGameGUI::DisableActionText()
{
    actionImg.SetIsVisible(false);
    actionText.SetIsVisible(false);
}

void InGameGUI::EnableScore(bool enabled)
{
    leftScoreText.SetIsVisible(enabled);
    rightScoreText.SetIsVisible(enabled);
    midScoreText.SetIsVisible(enabled);
}

void InGameGUI::EnableHUD(bool enabled)
{
    healthIcon.SetIsVisible(enabled);
    healthText.SetIsVisible(enabled);

    shieldIcon.SetIsVisible(enabled);
    armorText.SetIsVisible(enabled);

    ammoNumIcon.SetIsVisible(enabled);
    ammoText.SetIsVisible(enabled);

    wepIcon.SetIsVisible(enabled);
    altWepIcon.SetIsVisible(enabled);
    grenadeIcon.SetIsVisible(enabled);
    grenadeNumText.SetIsVisible(enabled);
    actionText.SetIsVisible(enabled);
    actionImg.SetIsVisible(enabled);

    flagIconImg.SetIsVisible(enabled);

    crosshairImg.SetIsVisible(enabled);
    hitmarkerImg.SetIsVisible(enabled);
    dmgEffectImg.SetIsVisible(enabled);

    leftScoreText.SetIsVisible(enabled);
    rightScoreText.SetIsVisible(enabled);
    midScoreText.SetIsVisible(enabled);
    gameTimeText.SetIsVisible(enabled);

    killText.SetIsVisible(enabled);
    respawnTimeText.SetIsVisible(enabled);
    countdownText.SetIsVisible(enabled);

    if(!enabled)
        DisableActionText();
}

void InGameGUI::EnableWinnerText(bool enabled)
{
    if(enabled)
    {
        auto mode = inGame->match.GetGameMode();
        auto scoreBoard = mode->GetScoreboard();
        auto winner = scoreBoard.GetWinner().value();

        auto color = inGame->teamColors[winner.teamId];
        std::string text = winner.teamId == TeamID::BLUE_TEAM_ID ? "BLUE TEAM" : "RED TEAM";

        auto isFFA = mode->GetType() == GameMode::FREE_FOR_ALL;
        if(isFFA)
        {
            color = inGame->ffaColors[winner.teamId];
            text = scoreBoard.GetPlayerScore(winner.teamId)->name;
        }

        winnerText.SetColor(color);
        winnerText.SetText(text);
        auto size = winnerText.GetSize();
        winnerText.SetOffset(-size / 2 );
    }

    winnerAnnoucerText.SetIsVisible(enabled);
    winnerText.SetIsVisible(enabled);
    EnableHUD(false);
}

void InGameGUI::EnableCapturingText(bool enabled)
{
    capturingText.SetIsVisible(enabled);
    captCountdownText.SetIsVisible(enabled);
}

void InGameGUI::SetCapturePercent(float percent)
{
    int value = std::ceil(percent);
    std::string text = std::to_string(value) + "%";
    captCountdownText.SetText(text);
}

void InGameGUI::ShowLogMsg(std::string msg)
{
    logText.SetText(msg);
    logAnimPlayer.Restart();
}

glm::vec4 InGameGUI::GetOppositeColor(Entity::ID playerTeam)
{
    auto color = inGame->teamColors[TeamID::BLUE_TEAM_ID];
    if(playerTeam == TeamID::BLUE_TEAM_ID)
        color = inGame->teamColors[RED_TEAM_ID];

    return color;
}

// Private

void InGameGUI::ScoreboardWindow()
{
    auto displaySize = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    ImGui::SetNextWindowSize(ImVec2{0, 0}, ImGuiCond_Always);

    auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground;
    if(ImGui::Begin("Score board", nullptr, flags))
    {   
        ImVec4 red{0.8f, 0.05f, 0.05f, 0.75f};
        ImVec4 blue{0.05f, 0.05f, 0.7f, 0.75f};
        ImVec4 yellow{0.921f, 0.886f, 0.058f, 0.75f};

        ImVec4 headerColor = ImGui::GetStyleColorVec4(ImGuiCol_TableHeaderBg);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, headerColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, headerColor);

        auto gameMode = inGame->match.GetGameMode();
        if(gameMode->GetType() == GameMode::FREE_FOR_ALL)
        {
            ScoreTable("Free For All");
        }
        else
        {   
            ImGui::PushStyleColor(ImGuiCol_TableRowBg, blue);
            ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, blue);
                ScoreTable("Blue Team", TeamID::BLUE_TEAM_ID);
            ImGui::PopStyleColor(2);
            
            ImGui::Dummy(ImVec2(0, 10));

            ImGui::PushStyleColor(ImGuiCol_TableRowBg, red);
            ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, red);
                ScoreTable("Red Team", TeamID::RED_TEAM_ID);
            ImGui::PopStyleColor(2);
        }
        ImGui::PopStyleColor(2);

        if(inGame->match.GetState() == Match::StateType::ENDED)
        {
            auto size = ImGui::CalcTextSize(" Leave Game ");
            auto regionSize = ImGui::GetContentRegionAvail();
            GUI::CenterSection(size.x, regionSize.x);
            if(ImGui::Button("Leave Game"))
            {
                inGame->Exit();
            }
        }
    }
    ImGui::End();
}

void InGameGUI::ScoreTable(const char* name, Entity::ID teamId)
{
    ImGui::Text("%s", name);
    auto winSize = ImGui::GetWindowSize();

    auto gameMode = inGame->match.GetGameMode();
    bool isFFA = gameMode->GetType() == GameMode::FREE_FOR_ALL;

    auto scoreBoard = gameMode->GetScoreboard();
    auto playerScores = isFFA ? scoreBoard.GetPlayerScores() : scoreBoard.GetTeamPlayers(teamId);
    std::sort(playerScores.begin(), playerScores.end(), [](auto a, auto b)
    {
        return a.score > b.score;
    });
    
    auto tFlags = ImGuiTableFlags_None | ImGuiTableFlags_RowBg;
    if(ImGui::BeginTable("#Score board", 7, tFlags))
    {
        auto columnFlags = ImGuiTableColumnFlags_WidthFixed;
        auto nameSize = winSize.x * 0.65f;
        ImGui::TableSetupColumn("Rank", columnFlags);
        ImGui::TableSetupColumn("Name", columnFlags, nameSize);
        ImGui::TableSetupColumn("Score", columnFlags);
        ImGui::TableSetupColumn("Kills", columnFlags);
        ImGui::TableSetupColumn("K/D", columnFlags);
        ImGui::TableSetupColumn("Deaths", columnFlags);
        ImGui::TableHeadersRow();

        // Team Row
        if(!isFFA)
        {
            ImGui::TableNextColumn();
            // Rank

            ImGui::TableNextColumn();
            ImGui::Text("%s", name);

            ImGui::TableNextColumn();
            auto teamScore = scoreBoard.GetTeamScore(teamId);
            int score = teamScore ? teamScore->score : 0;
            std::string points = std::to_string(score);
            auto width = ImGui::CalcTextSize(points.c_str()).x;
            GUI::TableCenterEntry(width);
            ImGui::Text("%s", points.c_str());

            // Total Row
            ImGui::TableNextRow();
        }
        
        // Players
        for(auto i = 0; i < playerScores.size(); i++)
        {
            auto playerScore = playerScores[i];
            auto teamId = playerScore.teamId;
            
            // Rank
            ImGui::TableNextColumn();
            std::string rank = std::to_string(i + 1);
            auto width = ImGui::CalcTextSize(rank.c_str()).x;
            GUI::TableCenterEntry(width);
            ImGui::Text("%s", rank.c_str());

            // Name
            ImGui::TableNextColumn();
            std::string playerName = playerScore.name;
            ImGui::Text("%s", playerName.c_str());

            // Points
            ImGui::TableNextColumn();
            std::string points = std::to_string(playerScore.score);
            width = ImGui::CalcTextSize(points.c_str()).x;
            GUI::TableCenterEntry(width);
            ImGui::Text("%s", points.c_str());

            // Kills
            ImGui::TableNextColumn();
            std::string kills = std::to_string(playerScore.kills);
            width = ImGui::CalcTextSize(kills.c_str()).x;
            GUI::TableCenterEntry(width);
            ImGui::Text("%s", kills.c_str());

            // K/D
            ImGui::TableNextColumn();
            auto r = (float)playerScore.kills / std::max(1.0f, (float)playerScore.deaths);
            std::stringstream stream;
            stream << std::fixed << std::setprecision(2) << r;
            std::string ratio = stream.str();
            width = ImGui::CalcTextSize(ratio.c_str()).x;
            GUI::TableCenterEntry(width);
            ImGui::Text("%s", ratio.c_str());

            // Deaths
            ImGui::TableNextColumn();
            std::string deaths = std::to_string(playerScore.deaths);
            width = ImGui::CalcTextSize(deaths.c_str()).x;
            GUI::TableCenterEntry(width);
            ImGui::Text("%s", deaths.c_str());

            if(isFFA)
            {
                auto color = inGame->ffaColors[teamId];
                auto ivec4 = ImVec4{color.x, color.y, color.z, color.w};
                ImGui::PushStyleColor(ImGuiCol_TableRowBg, ivec4);
                ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ivec4);
                if(teamId == InGame::FFA_WHITE)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0, 0, 0, 1});
            }
            ImGui::TableNextRow();
            if(isFFA)
            {
                ImGui::PopStyleColor(2);
                if(teamId == InGame::FFA_WHITE)
                    ImGui::PopStyleColor();
            }
            
        }

        ImGui::EndTable();
    }
}

#ifdef _DEBUG

void InGameGUI::DebugWindow()
{
    if(ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if(ImGui::CollapsingHeader("Transform"))
        {
            ImGui::InputInt("ID", (int*)&modelId);
            auto model = static_cast<Rendering::Model*>(inGame->modelMgr.GetModel(Game::Models::ROCKET_MODEL_ID));
            if(model)
            {
                if(auto sm = model->GetSubModel(modelId))
                {
                    auto& t = sm->transform;
                    ImGui::SliderFloat3("Offset", &t.position.x, -sliderPrecision, sliderPrecision);
                    ImGui::SliderFloat3("Scale", &t.scale.x, -sliderPrecision, sliderPrecision);
                    ImGui::SliderFloat3("Rotation", &t.rotation.x, -sliderPrecision, sliderPrecision);
                }
            }
            
            ImGui::InputFloat("Precision", &sliderPrecision);

            if(ImGui::Button("Apply"))
            {
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
                    playerState.shootPlayer.Restart();
                    break;
                }
            }
            if(ImGui::Button("Dmg"))
            {
                PlayScreenEffect();
            }
            if(ImGui::Button("Kill"))
            {
                PlayHitMarkerAnim(HitMarkerType::KILL);
            }

            auto zoom = inGame->camera_.GetZoom();
            if(ImGui::SliderFloat("Zoom", &zoom, 0.25, 2.5f))
            {
                inGame->camera_.SetZoom(zoom);
            }

            if(ImGui::Button("Create explosion"))
            {
                auto& player = inGame->GetLocalPlayer();
                inGame->explosionMgr.CreateExplosion(glm::vec3{0.0f, 10.0f, 0.0f});
            }
        }

        if(ImGui::CollapsingHeader("Audio"))
        {
            ImGui::Text("Audio");
            ImGui::Separator();

            auto source = inGame->grenadeSources.Get();
            auto params = inGame->audioMgr->GetSourceParams(source);
            if(ImGui::SliderFloat("Gain", &params->gain, 0.0f, 5.0f))
                inGame->audioMgr->SetSourceParams(source, *params);
            if(ImGui::SliderFloat("RelDistance", &params->relDistance, 0.0f, 25.f))
                inGame->audioMgr->SetSourceParams(source, *params);

            if(ImGui::Button("Play audio"))
            {
                inGame->audioMgr->PlayStreamSource(inGame->soundtrackSource);
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
            ImGui::Checkbox("Show", &showHitmarker);
        }
    }
    ImGui::End();
}

#endif

void InGameGUI::NetworkStatsWindow()
{
    if(ImGui::Begin("Network Stats"))
    {
        if(auto info = inGame->host.GetPeerInfo(inGame->serverId))
        {
            ImGui::Text("Config");
            ImGui::Separator();
            ImGui::Text("Server tick rate: %.2f ms", inGame->serverTickRate.count());

            ImGui::Text("Latency");
            ImGui::Separator();
            ImGui::Text("Ping:");ImGui::SameLine();ImGui::Text("%i ms", info->roundTripTimeMs);
            ImGui::Text("Ping variance: %i", info->roundTripTimeVariance);

            ImGui::Text("Packets");
            ImGui::Separator();
            ImGui::Text("Packets sent: %i", info->packetsSent);
            ImGui::Text("Packets ack: %i", info->packetsAck);
            ImGui::Text("Packets lost: %i", info->packetsLost);
            ImGui::Text("Packet loss: %i", info->packetLoss);

            ImGui::Text("Bandwidth");
            ImGui::Separator();
            ImGui::Text("In: %i B/s", info->incomingBandwidth);
            ImGui::Text("Out: %i B/s", info->outgoingBandwidth);
        }
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

// Handy 

Log::Logger* InGameGUI::GetLogger()
{
    return inGame->GetLogger();
}
