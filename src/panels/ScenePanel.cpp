#include "ScenePanel.h"
#include "render/Renderer.h"
#include "render/SceneRenderData.h"
#include "Widgets.h"
#include "UndoManager.h"
#include "Utils.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <imgui.h>
#include <IconsLucide.h>
#include <ImGuizmo.h>
#include <glm/gtc/constants.hpp>
#include <glm/ext.hpp>

namespace
{
    constexpr float CAMERA_ZOOM_MIN = 10.0f;
    constexpr float CAMERA_ZOOM_MAX = 500.0f;
    constexpr float CAMERA_ROTATION_SENSITIVITY = 0.005f;
    constexpr float CAMERA_PAN_SENSITIVITY = 0.1f;

    bool find_selected_with_parent_transform(
        Group& group,
        const glm::mat4& parent_transform,
        SceneNode*& out_node,
        glm::mat4& out_parent_transform,
        int& count)
    {
        for (auto& child : group.children)
        {
            if (!child) continue;

            if (child->selected)
            {
                out_node = child.get();
                out_parent_transform = parent_transform;
                ++count;
            }

            if (child->is_group())
            {
                glm::mat4 group_transform = parent_transform * render::build_node_transform(*child);
                find_selected_with_parent_transform(
                    *static_cast<Group*>(child.get()), group_transform, out_node, out_parent_transform, count);
            }
        }
        return count == 1;
    }
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

void ScenePanel::draw(Scene& scene, UndoManager* undo_manager)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(ICON_LC_GLOBE " Scene###ScenePanel");
    ImGui::PopStyleVar();

    if (ImGui::IsKeyPressed(ImGuiKey_G)) gizmo_operation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_R)) gizmo_operation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(ImGuiKey_S)) gizmo_operation = ImGuizmo::SCALE;

    ImVec2 viewport_size = ImGui::GetContentRegionAvail();
    int viewport_width  = std::max(1, static_cast<int>(viewport_size.x));
    int viewport_height = std::max(1, static_cast<int>(viewport_size.y));

    if (renderer)
        renderer->render(scene, viewport_width, viewport_height);

    if (renderer && renderer->get_texture_id() != 0)
    {
        ImGui::Image(
            renderer->get_texture_id(),
            ImVec2(static_cast<float>(viewport_width), static_cast<float>(viewport_height)),
            ImVec2(0.0f, 1.0f),
            ImVec2(1.0f, 0.0f));

        const ImVec2 image_min = ImGui::GetItemRectMin();
        const bool is_hovered = ImGui::IsItemHovered();

        const float toolbar_padding = 6.0f;
        const float button_size = 30.0f;
        const float button_spacing = ImGui::GetStyle().ItemSpacing.x;
        const int button_count = 3;
        const float toolbar_width  = button_count * button_size + (button_count - 1) * button_spacing + toolbar_padding * 2.0f;
        const float toolbar_height = button_size + toolbar_padding * 2.0f;

        ImVec2 toolbar_min(image_min.x + 8.0f, image_min.y + 8.0f);
        ImVec2 toolbar_max(toolbar_min.x + toolbar_width, toolbar_min.y + toolbar_height);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(toolbar_min, toolbar_max, ImGui::GetColorU32(ImGuiCol_WindowBg, 0.85f), 6.0f);
        draw_list->AddRect(toolbar_min, toolbar_max, ImGui::GetColorU32(ImGuiCol_Border), 6.0f);

        ImGui::SetCursorScreenPos(ImVec2(toolbar_min.x + toolbar_padding, toolbar_min.y + toolbar_padding));
        ImGui::BeginGroup();

        const ImVec4 button_color_active = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
        const ImVec4 button_color = ImGui::GetStyleColorVec4(ImGuiCol_Button);

        ImGui::PushStyleColor(ImGuiCol_Button, gizmo_operation == ImGuizmo::TRANSLATE ? button_color_active : button_color);
        if (ImGui::Button(ICON_LC_MOVE_3D, ImVec2(button_size, button_size))) gizmo_operation = ImGuizmo::TRANSLATE;
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, gizmo_operation == ImGuizmo::ROTATE ? button_color_active : button_color);
        if (ImGui::Button(ICON_LC_ROTATE_3D, ImVec2(button_size, button_size))) gizmo_operation = ImGuizmo::ROTATE;
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, gizmo_operation == ImGuizmo::SCALE ? button_color_active : button_color);
        if (ImGui::Button(ICON_LC_MAXIMIZE_2, ImVec2(button_size, button_size))) gizmo_operation = ImGuizmo::SCALE;
        ImGui::PopStyleColor();

        ImGui::EndGroup();

        const bool mouse_over_toolbar = ImGui::IsMouseHoveringRect(toolbar_min, toolbar_max);

        SceneNode* selected_node = nullptr;
        glm::mat4 parent_transform(1.0f);
        int selected_count = 0;
        find_selected_with_parent_transform(scene.root, glm::mat4(1.0f), selected_node, parent_transform, selected_count);

        bool gizmo_used_this_frame = false;

        if (selected_node)
        {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(image_min.x, image_min.y,
                            static_cast<float>(viewport_width), static_cast<float>(viewport_height));

            Camera& camera = renderer->get_camera();
            glm::mat4 view = camera.get_view_matrix();
            glm::mat4 projection = camera.get_projection_matrix(float(viewport_width) / float(viewport_height));

            if (!gizmo_was_using)
            {
                gizmo_active_matrix = parent_transform * render::build_node_transform(*selected_node);
            }

            float snap_translate[3] = { 1.0f, 1.0f, 1.0f };
            float snap_rotate[3]    = { 1.0f, 1.0f, 1.0f };
            const float* snap_values = nullptr;
            if (gizmo_operation == ImGuizmo::TRANSLATE) snap_values = snap_translate;
            else if (gizmo_operation == ImGuizmo::ROTATE) snap_values = snap_rotate;
            // для SCALE snap не передаём — см. предыдущее объяснение про кратные скачки

            if (ImGuizmo::Manipulate(
                    glm::value_ptr(view), glm::value_ptr(projection),
                    gizmo_operation, ImGuizmo::WORLD, glm::value_ptr(gizmo_active_matrix),
                    nullptr, snap_values))
            {
                if (!gizmo_was_using)
                {
                    if (undo_manager)
                        undo_manager->capture_snapshot(scene);

                    if (selected_node->is_item())
                    {
                        Item* item_for_snapshot = static_cast<Item*>(selected_node);
                        gizmo_scale_base_thickness = item_for_snapshot->thickness;
                        gizmo_scale_base_vertices  = item_for_snapshot->vertices;

                        if (!gizmo_scale_base_vertices.empty())
                        {
                            glm::i32vec2 min_v = gizmo_scale_base_vertices[0];
                            glm::i32vec2 max_v = gizmo_scale_base_vertices[0];
                            for (const auto& v : gizmo_scale_base_vertices)
                            {
                                min_v.x = std::min(min_v.x, v.x);
                                min_v.y = std::min(min_v.y, v.y);
                                max_v.x = std::max(max_v.x, v.x);
                                max_v.y = std::max(max_v.y, v.y);
                            }
                            gizmo_scale_base_size = max_v - min_v + glm::i32vec2(1, 1);
                        }
                        else
                        {
                            gizmo_scale_base_size = glm::i32vec2(1, 1);
                        }
                    }
                }

                glm::mat4 local_matrix = glm::inverse(parent_transform) * gizmo_active_matrix;

                float t[3], r[3], s[3];
                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(local_matrix), t, r, s);

                selected_node->position = glm::i32vec3(
                    static_cast<int>(std::lround(t[0])),
                    static_cast<int>(std::lround(t[1])),
                    static_cast<int>(std::lround(t[2])));
                selected_node->rotation = glm::i32vec3(
                    static_cast<int>(std::lround(r[0])),
                    static_cast<int>(std::lround(r[1])),
                    static_cast<int>(std::lround(r[2])));

                if (gizmo_operation == ImGuizmo::SCALE && selected_node->is_item())
                {
                    Item* item = static_cast<Item*>(selected_node);

                    item->thickness = std::max(1, static_cast<int>(std::lround(gizmo_scale_base_thickness * s[2])));

                    if (!gizmo_scale_base_vertices.empty())
                    {
                        glm::i32vec2 new_size(
                            std::max(1, static_cast<int>(std::lround(gizmo_scale_base_size.x * s[0]))),
                            std::max(1, static_cast<int>(std::lround(gizmo_scale_base_size.y * s[1]))));

                        item->vertices = gizmo_scale_base_vertices;
                        if (new_size != gizmo_scale_base_size)
                            RescaleVertices(item->vertices, gizmo_scale_base_size, new_size);
                    }
                }

                gizmo_used_this_frame = true;
            }
        }
        gizmo_was_using = gizmo_used_this_frame;
        if (!gizmo_used_this_frame)
            gizmo_scale_base_vertices.clear();

        const bool gizmo_blocking_input = ImGuizmo::IsOver() || ImGuizmo::IsUsing();
        
        if (is_hovered && !gizmo_blocking_input && !mouse_over_toolbar && ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))
        {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
            
            Camera& camera = renderer->get_camera();
            camera.yaw -= delta.x * CAMERA_ROTATION_SENSITIVITY;
            camera.pitch += delta.y * CAMERA_ROTATION_SENSITIVITY;
            
            const float pi_2 = glm::pi<float>() / 2.0f;
            camera.pitch = std::clamp(camera.pitch, -pi_2 + 0.1f, pi_2 - 0.1f);
            
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
        }

        if (is_hovered && !gizmo_blocking_input && !mouse_over_toolbar && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
        {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
            Camera& camera = renderer->get_camera();

            auto forward = camera.get_forward();
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

            float aspect = float(viewport_width) / float(viewport_height);

            float world_width = 2.0f * camera.distance * std::tan(camera.fov * 0.5f);
            float world_height = world_width / aspect;

            float pixel_world_x = world_width / float(viewport_width);
            float pixel_world_y = world_height / float(viewport_height);

            camera.target += right * (-delta.x * pixel_world_x) + up * (delta.y * pixel_world_y);

            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
        }

        if (is_hovered && ImGui::GetIO().MouseWheel != 0.0f)
        {
            Camera& camera = renderer->get_camera();
            
            float zoom_speed = std::max(1.0f, camera.distance * 0.05f);
            camera.distance -= ImGui::GetIO().MouseWheel * zoom_speed;
            camera.distance = std::clamp(camera.distance, CAMERA_ZOOM_MIN, CAMERA_ZOOM_MAX);
        }

        if (is_hovered && !gizmo_blocking_input && !mouse_over_toolbar && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            ImVec2 viewport_pos = ImGui::GetItemRectMin();
            
            float pixel_x = mouse_pos.x - viewport_pos.x;
            float pixel_y = mouse_pos.y - viewport_pos.y;
            
            float norm_x = pixel_x / viewport_width;
            float norm_y = pixel_y / viewport_height;
            
            float ndc_x = norm_x * 2.0f - 1.0f;
            float ndc_y = -(norm_y * 2.0f - 1.0f);
            
            Camera& camera = renderer->get_camera();
            glm::mat4 view = camera.get_view_matrix();
            glm::mat4 projection = camera.get_projection_matrix(float(viewport_width) / float(viewport_height));
            glm::mat4 inv_projection = glm::inverse(projection);
            glm::mat4 inv_view = glm::inverse(view);
            
            glm::vec4 ray_clip(ndc_x, ndc_y, -1.0f, 1.0f);
            glm::vec4 ray_eye = inv_projection * ray_clip;
            ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f);
            
            glm::vec3 ray_world = glm::normalize(glm::vec3(inv_view * ray_eye));
            glm::vec3 ray_origin = camera.get_eye_position();
            
            const Item* picked_item = renderer->pick_item(scene, ray_origin, ray_world);
            SceneNode* closest_node = picked_item ? const_cast<Item*>(picked_item) : nullptr;
            
            bool ctrl_pressed = ImGui::GetIO().KeyCtrl;
            
            if (closest_node)
            {
                if (!ctrl_pressed)
                {
                    std::function<void(Group&)> deselect_all = [&](Group& group)
                    {
                        group.selected = false;
                        for (auto& child : group.children)
                        {
                            if (!child) continue;
                            if (child->is_group())
                            {
                                deselect_all(*static_cast<Group*>(child.get()));
                            }
                            else
                            {
                                child->selected = false;
                            }
                        }
                    };
                    deselect_all(scene.root);
                }
                
                closest_node->selected = !closest_node->selected;
            }
            else if (!ctrl_pressed)
            {
                std::function<void(Group&)> deselect_all = [&](Group& group)
                {
                    group.selected = false;
                    for (auto& child : group.children)
                    {
                        if (!child) continue;
                        if (child->is_group())
                        {
                            deselect_all(*static_cast<Group*>(child.get()));
                        }
                        else
                        {
                            child->selected = false;
                        }
                    }
                };
                deselect_all(scene.root);
            }
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
