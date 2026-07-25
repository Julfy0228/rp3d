#pragma once

#include <vector>
#include <string>
#include <glm/vec2.hpp>

namespace Widgets
{
    bool ContourEdit(const char* label, std::vector<glm::i32vec2>& vertices, int size_x, int size_y);

    struct SceneTreeNodeFlags
    {
        bool is_group = false;
        bool is_selected = false;
        bool is_renaming = false;
        bool visible = true;
    };

    struct SceneTreeNodeOutput
    {
        bool visibility_toggled = false;
        bool open_toggled = false;
        bool clicked = false;
        bool right_clicked = false;
        bool name_double_clicked = false;
        bool rename_submitted = false;
    };

    SceneTreeNodeOutput SceneTreeNode(
        const char* str_id,
        SceneTreeNodeFlags flags,
        bool is_open,
        const char* icon,
        std::string& name
    );
}