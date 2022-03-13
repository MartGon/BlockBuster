#include <gui/GUI.h>

void GUI::HelpMarker(const char* text)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void GUI::CenterSection(float sWidth, float regionWidth)
{
    ImGui::SetCursorPosX(regionWidth/ 2.0f - sWidth / 2.0f);
}