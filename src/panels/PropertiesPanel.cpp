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

        ImGui::DragInt3("Position", &node->position.x, 0.1f);

        if (ImGui::DragInt3("Rotation", &node->rotation.x))
            for (int i = 0; i < 3; ++i)
                node->rotation[i] = (node->rotation[i] % 360 + 360) % 360;

        if (!node->is_group())
        {
            Item* item = static_cast<Item*>(node);

            if (ImGui::DragInt3("Size", &item->size.x, 0.1f))
                for (int i = 0; i < 3; ++i)
                    if (item->size[i] < 0)
                        item->size[i] = 0;

            glm::vec4 color_float = glm::vec4(item->color) / 255.0f;
            if (ImGui::ColorEdit4("Color", &color_float.x, ImGuiColorEditFlags_Uint8))
                item->color = glm::u8vec4(color_float * 255.0f + 0.5f);

            Widgets::ContourEdit("Contour", item->vertices, item->size.x, item->size.y);
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