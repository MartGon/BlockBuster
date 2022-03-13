#include <PopUp.h>

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
    {
        OnClose();
    }

    if(isVisible)
        ImGui::OpenPopup(titleStr);
}

void BasicPopUp::OnDraw()
{
    ImGui::Text("%s", text.c_str());

    auto winWidth = ImGui::GetWindowWidth();
    auto buttonWidth = ImGui::CalcTextSize("Accept").x + 8;
    ImGui::SetCursorPosX(winWidth / 2.0f - buttonWidth / 2.0f);
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