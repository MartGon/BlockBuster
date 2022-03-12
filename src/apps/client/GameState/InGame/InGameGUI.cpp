#include <GameState/InGame/InGameGUI.h>

#include <GameState/InGame/InGame.h>
#include <Client.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>

using namespace BlockBuster;

InGameGUI::InGameGUI(InGame& inGame) : inGame{&inGame}
{

}

void InGameGUI::Start()
{   
    // Init fonts
    std::filesystem::path fontPath = std::filesystem::path{RESOURCES_DIR} / "fonts/Pixel.ttf";
    pixelFont = GUI::TextFactory::Get()->LoadFont(fontPath);
    text = pixelFont->CreateText();
    text.SetText("See ya");

    // Menu PopUp
    GUI::GenericPopUp menu;
    auto onDrawMenu = [this](){
        ImGui::Button("Resume");
        if(ImGui::Button("Options"))
            this->puMgr.SetCur(PopUpState::OPTIONS);
        ImGui::Button("Exit Game");
    };
    menu.SetOnDraw(onDrawMenu);
    menu.SetVisible(true);
    menu.SetCloseable(true);
    puMgr.Set(MENU, menu);

    // Options Pop Up
    GUI::GenericPopUp options;
    auto onDrawOptions = [this](){
        ImGui::SliderFloat("Sensitivity", &sensitivity, 0.1f, 5.0f, "%.2f");
        ImGui::Checkbox("Sound enabled", &sound);
    };
    options.SetOnClose([this](){
        this->puMgr.SetCur(PopUpState::MENU);
    });
    options.SetOnDraw(onDrawOptions);
    options.SetVisible(true);
    options.SetCloseable(true);
    puMgr.Set(OPTIONS, options);

    puMgr.SetCur(MENU);
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
        puMgr.DrawCur();
    }
    
    // Draw GUI
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData(), guiVao.GetHandle());
    auto windowSize = inGame->client_->GetWindowSize();
    int scissor_box[4] = { 0, 0, windowSize.x, windowSize.y };
    ImGui_ImplOpenGL3_RestoreState(scissor_box);

    text.SetColor(glm::vec4{0.0f, 1.0f, 0.0f, 1.0f});
    text.SetScale(2.0f);
    text.Draw(textShader, glm::vec2{0.0f, 0.0f}, glm::vec2{(float)windowSize.x, (float)windowSize.y});
}

void InGameGUI::DebugWindow()
{
    if(ImGui::Begin("Transform"))
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
