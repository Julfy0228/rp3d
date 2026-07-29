#include "ScenePanel.h"
#include "render/Renderer.h"
#include "Widgets.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <IconsLucide.h>
#include <glm/gtc/constants.hpp>
#include <glm/ext.hpp>

namespace
{
    constexpr float CAMERA_ZOOM_MIN = 10.0f;
    constexpr float CAMERA_ZOOM_MAX = 500.0f;
    constexpr float CAMERA_ROTATION_SENSITIVITY = 0.005f;
    constexpr float CAMERA_PAN_SENSITIVITY = 0.1f;
}

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

        const bool is_hovered = ImGui::IsItemHovered();
        
        if (is_hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))
        {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
            
            Camera& camera = renderer->get_camera();
            camera.yaw -= delta.x * CAMERA_ROTATION_SENSITIVITY;
            camera.pitch += delta.y * CAMERA_ROTATION_SENSITIVITY;
            
            const float pi_2 = glm::pi<float>() / 2.0f;
            camera.pitch = std::clamp(camera.pitch, -pi_2 + 0.1f, pi_2 - 0.1f);
            
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
        }

        if (is_hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
        {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
            Camera& camera = renderer->get_camera();

            const float cos_yaw = std::cos(camera.yaw);
            const float sin_yaw = std::sin(camera.yaw);
            const float cos_pitch = std::cos(camera.pitch);
            const float sin_pitch = std::sin(camera.pitch);
            glm::vec3 forward(cos_yaw * cos_pitch, sin_yaw * cos_pitch, sin_pitch);
            float len_fwd = glm::length(forward);
            if (len_fwd > 0.0f) forward /= len_fwd;

            glm::vec3 world_up(0.0f, 0.0f, 1.0f);
            glm::vec3 right = glm::cross(forward, world_up);
            float len_right = glm::length(right);
            if (len_right < 0.001f)
                right = glm::vec3(1.0f, 0.0f, 0.0f);
            else
                right /= len_right;
            glm::vec3 up = glm::cross(right, forward);

            float fov = camera.fov;
            float aspect = static_cast<float>(viewport_width) / static_cast<float>(viewport_height);

            float world_width = 2.0f * camera.distance * std::tan(fov * 0.5f);
            float world_height = world_width / aspect;

            float pixel_world_x = world_width / static_cast<float>(viewport_width);
            float pixel_world_y = world_height / static_cast<float>(viewport_height);

            camera.target += right * (delta.x * pixel_world_x) + up * (delta.y * pixel_world_y);

            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
        }

        if (is_hovered && ImGui::GetIO().MouseWheel != 0.0f)
        {
            Camera& camera = renderer->get_camera();
            
            float zoom_speed = std::max(1.0f, camera.distance * 0.05f);
            camera.distance -= ImGui::GetIO().MouseWheel * zoom_speed;
            camera.distance = std::clamp(camera.distance, CAMERA_ZOOM_MIN, CAMERA_ZOOM_MAX);
        }

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
