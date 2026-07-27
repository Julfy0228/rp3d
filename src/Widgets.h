#pragma once

#include <vector>
#include <string>
#include <functional>
#include <glm/vec2.hpp>
#include <imgui.h>

namespace Widgets
{
    void DrawCenterMarker(ImDrawList* draw_list, ImVec2 center_pos, float circle_radius = 4.0f, float cross_length = 8.0f);

    bool ContourEdit(
        const char* label,
        std::vector<glm::i32vec2>& vertices,
        const std::function<void()>& on_edit_begin = {},
        const std::function<void()>& on_edit_end = {});

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