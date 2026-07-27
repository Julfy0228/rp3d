#pragma once

#include "render/Renderer.h"

#include <vector>

namespace render
{
    void collect_items(const Group& group, const glm::mat4& parent_transform, std::vector<Renderer::ItemRenderData>& items);
    void collect_selected_roots_for_viewport(const Group& group, std::vector<const SceneNode*>& selected_nodes, bool ancestor_selected);
    glm::mat4 build_node_transform(const SceneNode& node);
    glm::mat4 build_model_matrix(const Item& item, const glm::mat4& parent_transform, const glm::vec3& pivot);
}