#include <PopUp.h>

#include <gui/GUI.h>

#include <cstring>

using namespace GUI;

void PopUp::Draw()
{  
    auto displaySize = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});

    auto titleStr = title.c_str();
    if(isVisible && !ImGui::IsPopupOpen(titleStr))
    {
        ImGui::OpenPopup(titleStr);
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

void EditTextPopUp::OnDraw()
{
    auto capacity = textBuffer.capacity();
    bool accept = ImGui::InputText(label.c_str(), textBuffer.data(), capacity, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsNoBlank);
    bool empty = std::strlen(textBuffer.data()) == 0;

    if((ImGui::Button("Accept") || accept) && !empty)
    {
        bool res = onAccept(textBuffer.data());

        if(res)
        {
            onError = false;
            
            Close();
        }
        else
        {
            onError = true;
        }
    }

    ImGui::SameLine();
    if(ImGui::Button("Cancel"))
    {
        if(onCancel)
            onCancel();

        Close();
    }

    if(onError)
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", errorText.c_str());
}

void EditTextPopUp::OnOpen()
{
    if(onOpen)
        onOpen();
}

void EditTextPopUp::OnClose()
{
    if(onClose)
        onClose();
}