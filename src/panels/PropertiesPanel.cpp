#include "PropertiesPanel.h"
#include "Widgets.h"

#include "../UndoManager.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <IconsLucide.h>
#include <Colors.h>

#include <algorithm>

void PropertiesPanel::collect_selected_roots(SceneNode* node, std::vector<SceneNode*>& selected_nodes, bool ancestor_selected)
{
    if (!node)
        return;

    const bool node_selected = node->selected;
    if (node_selected && !ancestor_selected)
        selected_nodes.push_back(node);

    if (!node->is_group())
        return;

    Group* group = static_cast<Group*>(node);
    for (auto& child : group->children)
        collect_selected_roots(child.get(), selected_nodes, ancestor_selected || node_selected);
}

glm::i32vec3 PropertiesPanel::compute_selection_center(const std::vector<SceneNode*>& selected_roots) const
{
    if (selected_roots.empty())
        return glm::i32vec3(0);

    glm::vec3 sum(0.0f);
    for (SceneNode* node : selected_roots)
        sum += glm::vec3(node->position);

    const glm::vec3 average = sum / static_cast<float>(selected_roots.size());
    return glm::i32vec3(
        static_cast<int>(average.x >= 0.0f ? average.x + 0.5f : average.x - 0.5f),
        static_cast<int>(average.y >= 0.0f ? average.y + 0.5f : average.y - 0.5f),
        static_cast<int>(average.z >= 0.0f ? average.z + 0.5f : average.z - 0.5f));
}

void PropertiesPanel::capture_undo_once(Scene& scene, UndoManager* undo_manager, bool& snapshot_captured) const
{
    if (!snapshot_captured && undo_manager)
    {
        undo_manager->capture_snapshot(scene);
        snapshot_captured = true;
    }
}

void PropertiesPanel::draw(Scene& scene, UndoManager* undo_manager)
{
    ImGui::Begin(ICON_LC_TABLE_PROPERTIES "Properties###PropertiesPanel");

    std::vector<SceneNode*> selected_nodes;
    for (auto& child : scene.root.children)
        collect_selected_roots(child.get(), selected_nodes, false);

    if (selected_nodes.size() == 1)
    {
        SceneNode* node = selected_nodes.front();
        bool transform_snapshot_captured = false;
        bool contour_snapshot_captured = false;

        ImGui::Text(ICON_LC_SCAN "Transform");
        ImGui::DragInt3(ICON_LC_MOVE_3D "Position", &node->position.x, 0.1f, 0, 0, "%d", ImGuiSliderFlags_ColorMarkers);
        if (ImGui::IsItemActivated())
            capture_undo_once(scene, undo_manager, transform_snapshot_captured);

        if (ImGui::DragInt3(ICON_LC_ROTATE_3D "Rotation", &node->rotation.x, (1.0F), 0, 0, "%d", ImGuiSliderFlags_ColorMarkers))
        {
            for (int i = 0; i < 3; ++i)
                node->rotation[i] = (node->rotation[i] % 360 + 360) % 360;
        }
        if (ImGui::IsItemActivated())
            capture_undo_once(scene, undo_manager, transform_snapshot_captured);

        if (!node->is_group())
        {
            Item* item = static_cast<Item*>(node);

            ImGui::SetNextItemColorMarker(IM_COL32(20,20,240,255));
            if (ImGui::DragInt(ICON_LC_SCALING "Thickness", &item->thickness, 0.1f))
            {
                if (item->thickness < 0) item->thickness = 0;
            }
            if (ImGui::IsItemActivated())
                capture_undo_once(scene, undo_manager, transform_snapshot_captured);
            
            ImGui::Spacing();
            Widgets::ContourEdit(
                ICON_LC_VECTOR_SQUARE "Contour",
                item->vertices,
                [&]() { capture_undo_once(scene, undo_manager, contour_snapshot_captured); },
                [&]() { contour_snapshot_captured = false; });

            ImGui::Spacing();
            ImGui::Text(ICON_LC_PIPETTE "Color");
            glm::vec4 color_float = glm::vec4(item->color) / 255.0f;
            if (ImGui::ColorEdit4("##Color", &color_float.x, ImGuiColorEditFlags_Uint8))
                item->color = glm::u8vec4(color_float * 255.0f + 0.5f);
            if (ImGui::IsItemActivated())
                capture_undo_once(scene, undo_manager, transform_snapshot_captured);
        }
        else
        {
            ImGui::TextDisabled("Group Node (Children: %d)", (int)static_cast<Group*>(node)->children.size());
        }
    }
    else if (selected_nodes.size() > 1)
    {
        ImGui::Text("%d items selected", (int)selected_nodes.size());

        glm::i32vec3 selection_center = compute_selection_center(selected_nodes);
        glm::i32vec3 previous_center = selection_center;
        bool transform_snapshot_captured = false;
        if (ImGui::DragInt3(ICON_LC_MOVE_3D "Position", &selection_center.x, 0.1f, 0, 0, "%d", ImGuiSliderFlags_ColorMarkers))
        {
            const glm::i32vec3 delta = selection_center - previous_center;
            for (SceneNode* node : selected_nodes)
                node->position += delta;
        }
        if (ImGui::IsItemActivated())
            capture_undo_once(scene, undo_manager, transform_snapshot_captured);
    }

    ImGui::End();
}