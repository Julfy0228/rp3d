#include "ScenePanel.h"
#include "render/Renderer.h"
#include "Widgets.h"

#include <algorithm>
#include <imgui.h>
#include <IconsLucide.h>

ScenePanel::ScenePanel() = default;

ScenePanel::~ScenePanel()
{
    shutdown();
}

bool ScenePanel::init()
{
    if (!renderer)
        renderer = std::make_unique<Renderer>();

    return renderer->init();
}

void ScenePanel::shutdown()
{
    if (!renderer)
        return;

    renderer->shutdown();
    renderer.reset();
}

void ScenePanel::draw(Scene& scene)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(ICON_LC_GLOBE " Scene###ScenePanel");
    ImGui::PopStyleVar();

    ImVec2 viewport_size = ImGui::GetContentRegionAvail();
    int viewport_width  = std::max(1, static_cast<int>(viewport_size.x));
    int viewport_height = std::max(1, static_cast<int>(viewport_size.y));

    if (renderer)
    {
        renderer->render(scene, viewport_width, viewport_height);
    }

    if (renderer && renderer->get_texture_id() != 0)
    {
        ImGui::Image(
            renderer->get_texture_id(),
            ImVec2(static_cast<float>(viewport_width), static_cast<float>(viewport_height)),
            ImVec2(0.0f, 1.0f),
            ImVec2(1.0f, 0.0f));

        if (std::optional<glm::vec2> marker_position = renderer->get_selection_center_screen_position())
        {
            const ImVec2 image_min = ImGui::GetItemRectMin();
            const ImVec2 center_pos(image_min.x + marker_position->x, image_min.y + marker_position->y);
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            Widgets::DrawCenterMarker(draw_list, center_pos);
        }
    }
    else
    {
        ImGui::TextDisabled("Viewport is not ready");
    }

    ImGui::End();
}
