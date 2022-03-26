#include <gui/GUI.h>

void GUI::HelpMarker(const char* text)
{
    ImGui::TextDisabled("(?)");
    GUI::AddToolTip(text);
}

void GUI::AddToolTip(const char* text)
{
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

void GUI::TableCenterEntry(float width)
{
    auto regionSize = ImGui::GetContentRegionAvail();
    auto cursorPos = ImGui::GetCursorPos();
    auto x = cursorPos.x + regionSize.x / 2 - width /2;
    ImGui::SetCursorPosX(x);
}

void GUI::ImGuiImage(ImTextureID textureId, glm::ivec2 size)
{
    auto aspect = (float)size.y / (float)size.x;
    auto regionWidth = ImGui::GetContentRegionAvail().x;
    ImGui::Image(textureId, ImVec2{regionWidth, regionWidth * aspect});
}