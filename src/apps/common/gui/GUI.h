#pragma once

#include <imgui/imgui.h>

namespace GUI
{
    void HelpMarker(const char* text);
    void AddToolTip(const char* text);

    void CenterSection(float sWidth, float regionWidth);
    void TableCenterEntry(float width);

    void ImGuiImage(ImTextureID texure_id, glm::ivec2 imgSize);
}