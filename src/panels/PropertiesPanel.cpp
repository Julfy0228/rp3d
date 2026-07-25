#include "PropertiesPanel.h"
#include "Widgets.h"

#include <imgui.h>
#include <IconsLucide.h>

void PropertiesPanel::collect_selected(SceneNode* node, std::vector<SceneNode*>& selected_nodes)
{
    if (!node)
        return;

    if (node->selected)
        selected_nodes.push_back(node);

    if (!node->is_group())
        return;

    Group* group = static_cast<Group*>(node);
    for (auto& child : group->children)
        collect_selected(child.get(), selected_nodes);
}

void PropertiesPanel::draw(Scene& scene)
{
    ImGui::Begin(ICON_LC_TABLE_PROPERTIES "Properties###PropertiesPanel");

    std::vector<SceneNode*> selected_nodes;
    for (auto& child : scene.root.children)
        collect_selected(child.get(), selected_nodes);

    if (selected_nodes.size() == 1)
    {
        SceneNode* node = selected_nodes.front();

        ImGui::Text(ICON_LC_SCAN "Transform");
        ImGui::DragInt3(ICON_LC_MOVE_3D "Position", &node->position.x, 0.1f);

        if (ImGui::DragInt3(ICON_LC_ROTATE_3D "Rotation", &node->rotation.x))
            for (int i = 0; i < 3; ++i)
                node->rotation[i] = (node->rotation[i] % 360 + 360) % 360;

        if (!node->is_group())
        {
            Item* item = static_cast<Item*>(node);

            if (ImGui::DragInt(ICON_LC_SCALING "Thickness", &item->thickness, 0.1f))
                if (item->thickness < 0) item->thickness = 0;
            
            ImGui::Spacing();
            Widgets::ContourEdit(ICON_LC_VECTOR_SQUARE "Contour", item->vertices);

            ImGui::Spacing();
            ImGui::Text(ICON_LC_PIPETTE "Color");
            glm::vec4 color_float = glm::vec4(item->color) / 255.0f;
            if (ImGui::ColorEdit4("##Color", &color_float.x, ImGuiColorEditFlags_Uint8))
                item->color = glm::u8vec4(color_float * 255.0f + 0.5f);
        }
        else
        {
            ImGui::TextDisabled("Group Node (Children: %d)", (int)static_cast<Group*>(node)->children.size());
        }
    }
    else if (selected_nodes.size() > 1)
    {
        ImGui::Text("%d items selected", (int)selected_nodes.size());
        // TODO: Сделать групповое смещение
    }

    ImGui::End();
}