#include "SceneRenderData.h"
#include "MeshBuilder.h"

#include <glm/ext/matrix_transform.hpp>

namespace render
{
    glm::mat4 build_node_transform(const SceneNode& node)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(node.position.x, node.position.y, node.position.z));
        transform = glm::rotate(transform, glm::radians(static_cast<float>(node.rotation.z)), glm::vec3(0.0f, 0.0f, 1.0f));
        transform = glm::rotate(transform, glm::radians(static_cast<float>(node.rotation.y)), glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::rotate(transform, glm::radians(static_cast<float>(node.rotation.x)), glm::vec3(1.0f, 0.0f, 0.0f));
        return transform;
    }

    glm::mat4 build_model_matrix(const Item& item, const glm::mat4& parent_transform, const glm::vec3& pivot)
    {
        (void)pivot;
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(item.position));
        transform = glm::rotate(transform, glm::radians(static_cast<float>(item.rotation.z)), glm::vec3(0.0f, 0.0f, 1.0f));
        transform = glm::rotate(transform, glm::radians(static_cast<float>(item.rotation.y)), glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::rotate(transform, glm::radians(static_cast<float>(item.rotation.x)), glm::vec3(1.0f, 0.0f, 0.0f));
        return parent_transform * transform;
    }

    void collect_items(const Group& group, const glm::mat4& parent_transform, std::vector<Renderer::ItemRenderData>& items)
    {
        glm::mat4 group_transform = parent_transform;
        if (group.id != 0)
            group_transform = parent_transform * build_node_transform(group);

        for (const auto& child : group.children)
        {
            if (!child || !child->visible)
                continue;

            if (child->is_group())
            {
                collect_items(*static_cast<const Group*>(child.get()), group_transform, items);
            }
            else
            {
                Renderer::ItemRenderData item_data;
                item_data.item = static_cast<const Item*>(child.get());
                item_data.pivot = compute_item_pivot(*item_data.item);
                item_data.model = build_model_matrix(*item_data.item, group_transform, item_data.pivot);
                item_data.world_center = glm::vec3(item_data.model[3]);
                items.push_back(item_data);
            }
        }
    }

    void collect_selected_roots_for_viewport(const Group& group, std::vector<const SceneNode*>& selected_nodes, bool ancestor_selected)
    {
        for (const auto& child : group.children)
        {
            if (!child || !child->visible)
                continue;

            const bool child_selected = child->selected;
            if (child_selected && !ancestor_selected)
                selected_nodes.push_back(child.get());

            if (child->is_group())
            {
                collect_selected_roots_for_viewport(
                    *static_cast<const Group*>(child.get()),
                    selected_nodes,
                    ancestor_selected || child_selected);
            }
        }
    }
}