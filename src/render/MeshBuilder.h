#pragma once

#include "render/Renderer.h"

#include <cstdint>
#include <vector>

namespace render
{
    std::uint64_t compute_item_mesh_signature(const Item& item);
    glm::vec3 compute_item_pivot(const Item& item);
    bool build_item_mesh(
        const Item& item,
        std::vector<Renderer::MeshVertex>& vertices,
        std::vector<unsigned int>& indices,
        glm::vec3& pivot);
}