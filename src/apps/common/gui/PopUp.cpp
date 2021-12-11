#include <PopUp.h>

using namespace GUI;

void PopUp::Draw()
{  
    auto displaySize = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    if(ImGui::BeginPopupModal(title.c_str(), &isVisible, flags))
    {
        ImGui::Text("%s", text.c_str());

        if(isButtonVisible && ImGui::Button("Ok"))
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