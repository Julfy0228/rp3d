#include "StylePanel.h"

#include <imgui.h>
#include <IconsLucide.h>

void StylePanel::draw()
{
    if (!show) return;
    if (ImGui::Begin(ICON_LC_PALETTE "Style###StylePanel", &show))
        ImGui::ShowStyleEditor();
    ImGui::End();
}