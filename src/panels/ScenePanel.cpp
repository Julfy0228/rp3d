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

        if (std::optional<glm::vec2> marker_position = viewport_renderer->get_selection_center_screen_position())
        {
            const ImVec2 image_min = ImGui::GetItemRectMin();
            const ImVec2 center_pos(image_min.x + marker_position->x, image_min.y + marker_position->y);
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddCircleFilled(center_pos, 4.0f, IM_COL32(120, 255, 120, 255));
            draw_list->AddLine(ImVec2(center_pos.x - 8.0f, center_pos.y), ImVec2(center_pos.x + 8.0f, center_pos.y), IM_COL32(120, 255, 120, 220), 1.5f);
            draw_list->AddLine(ImVec2(center_pos.x, center_pos.y - 8.0f), ImVec2(center_pos.x, center_pos.y + 8.0f), IM_COL32(120, 255, 120, 220), 1.5f);
        }
    }
    else
    {
        ImGui::TextDisabled("Viewport is not ready");
    }

    ImGui::End();
}