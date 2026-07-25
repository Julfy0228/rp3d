#include "ScenePanel.h"
#include "ViewportRenderer.h"

#include <imgui.h>
#include <IconsLucide.h>

void ScenePanel::draw(Scene& scene, ViewportRenderer* viewport_renderer)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(ICON_LC_GLOBE "Scene###ScenePanel");
    ImGui::PopStyleVar();

    ImVec2 viewport_size = ImGui::GetContentRegionAvail();
    int viewport_width = std::max(1, static_cast<int>(viewport_size.x));
    int viewport_height = std::max(1, static_cast<int>(viewport_size.y));

    if (viewport_renderer)
    {
        viewport_renderer->render(scene, viewport_width, viewport_height);
    }

    if (viewport_renderer && viewport_renderer->get_texture_id() != 0)
    {
        ImGui::Image(
            viewport_renderer->get_texture_id(),
            ImVec2(static_cast<float>(viewport_width), static_cast<float>(viewport_height)),
            ImVec2(0.0f, 1.0f),
            ImVec2(1.0f, 0.0f));
    }
    else
    {
        ImGui::TextDisabled("Viewport is not ready");
    }

    ImGui::End();
}