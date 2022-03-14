#include <PopUp.h>

#include <gui/GUI.h>

using namespace GUI;

void PopUp::Draw()
{  
    auto displaySize = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});

    auto titleStr = title.c_str();
    if(isVisible && !ImGui::IsPopupOpen(titleStr))
    {
        ImGui::OpenPopup(titleStr);
        OnOpen();
    }

    bool* showClose = isCloseable ? &isVisible : nullptr;
    bool wasOpen = isVisible;
    if(ImGui::BeginPopupModal(titleStr, showClose, flags))
    {
        if(!isVisible)
            ImGui::CloseCurrentPopup();

        OnDraw();

        ImGui::EndPopup();
    }
    else if(wasOpen)
        OnClose();
}

void BasicPopUp::OnDraw()
{
    ImGui::Text("%s", text.c_str());

    auto winWidth = ImGui::GetWindowWidth();
    auto buttonWidth = ImGui::CalcTextSize("Accept").x + 8;
    GUI::CenterSection(buttonWidth, winWidth);
    if(isButtonVisible && ImGui::Button("Accept"))
    {
        SetVisible(false);

        if(onButtonPress)
            onButtonPress();
    }

}

void GenericPopUp::OnDraw()
{
    if(content)
        content();
}

void GenericPopUp::OnOpen()
{
    if(onOpen)
        onOpen();
}

void GenericPopUp::OnClose()
{
    if(onClose)
        onClose();
}