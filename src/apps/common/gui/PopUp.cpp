#include <PopUp.h>

using namespace GUI;

void PopUp::Draw()
{  
    auto displaySize = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});

    bool* showClose = isCloseable ? &isVisible : nullptr;
    if(ImGui::BeginPopupModal(title.c_str(), showClose, flags))
    {
        if(!isVisible)
            ImGui::CloseCurrentPopup();

        ImGui::Text("%s", text.c_str());

        auto winWidth = ImGui::GetWindowWidth();
        auto buttonWidth = ImGui::CalcTextSize("Accept").x + 8;
        ImGui::SetCursorPosX(winWidth / 2.0f - buttonWidth / 2.0f);
        if(isButtonVisible && ImGui::Button("Accept"))
        {
            isVisible = false;

            if(onButtonPress)
                onButtonPress();
        }

        ImGui::EndPopup();
    }

    if(isVisible)
        ImGui::OpenPopup(title.c_str());
}