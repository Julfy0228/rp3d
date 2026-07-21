#pragma once

#include <vector>

#include <glm/vec2.hpp>

namespace Widgets
{
    bool ContourEdit(const char* label, std::vector<glm::i32vec2>& vertices, int size_x, int size_y);
}