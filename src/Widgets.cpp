#include "Widgets.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <imgui.h>
#include <glm/common.hpp>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <IconsLucide.h>

namespace Widgets
{
    bool ContourEdit(const char* label, std::vector<glm::i32vec2>& vertices)
    {
        bool value_changed = false;
        glm::vec2 geometry_center(0.0f, 0.0f);

        static glm::i32vec2 user_grid_size = glm::i32vec2(8, 8);

        int min_x = 0, max_x = 0, min_y = 0, max_y = 0;
        int req_width = 1;
        int req_height = 1;

        if (!vertices.empty())
        {
            min_x = max_x = vertices[0].x;
            min_y = max_y = vertices[0].y;
            for (const auto& pt : vertices)
            {
                min_x = std::min(min_x, pt.x);
                max_x = std::max(max_x, pt.x);
                min_y = std::min(min_y, pt.y);
                max_y = std::max(max_y, pt.y);
            }
            geometry_center = glm::vec2((min_x + max_x) * 0.5f, (min_y + max_y) * 0.5f);

            req_width = std::max(1, max_x);
            req_height = std::max(1, max_y);
        }

        ImGui::PushID(label);

        static const void* last_vertices_key = nullptr;
        const void* current_vertices_key = static_cast<const void*>(&vertices);
        if (last_vertices_key != current_vertices_key)
        {
            last_vertices_key = current_vertices_key;
            if (!vertices.empty())
            {
                user_grid_size.x = req_width;
                user_grid_size.y = req_height;
            }
            else
            {
                user_grid_size = glm::i32vec2(8, 8);
            }
        }

        user_grid_size.x = std::clamp(user_grid_size.x, req_width, 512);
        user_grid_size.y = std::clamp(user_grid_size.y, req_height, 512);

        ImGui::Text("%s", label);

        ImGui::PushItemWidth(120.0f);
        if (ImGui::DragInt2(ICON_LC_MAXIMIZE_2 "Size", &user_grid_size.x, 0.2f, req_width, 512, "%d", ImGuiSliderFlags_ColorMarkers))
        {
            user_grid_size.x = std::clamp(user_grid_size.x, req_width, 512);
            user_grid_size.y = std::clamp(user_grid_size.y, req_height, 512);
            value_changed = true;
        }
        ImGui::PopItemWidth();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
        if (ImGui::Button(ICON_LC_ERASER "Clear"))
        {
            vertices.clear();
            value_changed = true;
        }
        ImGui::PopStyleColor();

        int active_grid_x = user_grid_size.x;
        int active_grid_y = user_grid_size.y;

        if (active_grid_x > 0 && active_grid_y > 0)
        {
            const float padding = 8.0f;
            ImVec2 avail_sz = ImGui::GetContentRegionAvail();
            if (avail_sz.x < 50.0f) avail_sz.x = 50.0f;

            float aspect = static_cast<float>(active_grid_y) / static_cast<float>(active_grid_x);
            ImVec2 canvas_sz(avail_sz.x, avail_sz.x * aspect);

            ImVec2 canvas_p = ImGui::GetCursorScreenPos();
            ImGuiIO& io = ImGui::GetIO();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            draw_list->AddRectFilled(canvas_p, ImVec2(canvas_p.x + canvas_sz.x, canvas_p.y + canvas_sz.y), IM_COL32(30, 30, 35, 255));
            draw_list->AddRect(canvas_p, ImVec2(canvas_p.x + canvas_sz.x, canvas_p.y + canvas_sz.y), IM_COL32(70, 70, 80, 255));

            ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
            const bool is_hovered = ImGui::IsItemHovered();

            ImVec2 inner_p(canvas_p.x + padding, canvas_p.y + padding);
            ImVec2 inner_sz(canvas_sz.x - padding * 2.0f, canvas_sz.y - padding * 2.0f);

            auto GridToScreen = [&](const glm::i32vec2& pt) -> ImVec2
            {
                float norm_x = static_cast<float>(pt.x) / static_cast<float>(active_grid_x);
                float norm_y = static_cast<float>(pt.y) / static_cast<float>(active_grid_y);
                return ImVec2(inner_p.x + norm_x * inner_sz.x, inner_p.y + norm_y * inner_sz.y);
            };

            auto GridToScreenFloat = [&](const glm::vec2& pt) -> ImVec2
            {
                float norm_x = pt.x / static_cast<float>(active_grid_x);
                float norm_y = pt.y / static_cast<float>(active_grid_y);
                return ImVec2(inner_p.x + norm_x * inner_sz.x, inner_p.y + norm_y * inner_sz.y);
            };

            auto ScreenToGrid = [&](const ImVec2& pos) -> glm::i32vec2
            {
                float norm_x = (pos.x - inner_p.x) / inner_sz.x;
                float norm_y = (pos.y - inner_p.y) / inner_sz.y;
                int gx = static_cast<int>(std::round(norm_x * active_grid_x));
                int gy = static_cast<int>(std::round(norm_y * active_grid_y));
                return glm::i32vec2(glm::clamp(gx, 0, active_grid_x), glm::clamp(gy, 0, active_grid_y));
            };

            int step_x = std::max(1, active_grid_x / 16);
            int step_y = std::max(1, active_grid_y / 16);

            for (int x = 0; x <= active_grid_x; x += step_x)
            {
                ImVec2 p1 = GridToScreen({x, 0});
                ImVec2 p2 = GridToScreen({x, active_grid_y});
                draw_list->AddLine(p1, p2, IM_COL32(50, 50, 60, 150));
            }
            for (int y = 0; y <= active_grid_y; y += step_y)
            {
                ImVec2 p1 = GridToScreen({0, y});
                ImVec2 p2 = GridToScreen({active_grid_x, y});
                draw_list->AddLine(p1, p2, IM_COL32(50, 50, 60, 150));
            }

            static int dragged_point_idx = -1;
            const float node_radius = 6.0f;

            if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                dragged_point_idx = -1;
                for (int i = 0; i < static_cast<int>(vertices.size()); ++i)
                {
                    ImVec2 node_pos = GridToScreen(vertices[i]);
                    float dist_sq = (io.MousePos.x - node_pos.x) * (io.MousePos.x - node_pos.x) +
                                    (io.MousePos.y - node_pos.y) * (io.MousePos.y - node_pos.y);
                    if (dist_sq <= (node_radius + 4.0f) * (node_radius + 4.0f))
                    {
                        dragged_point_idx = i;
                        break;
                    }
                }

                if (dragged_point_idx == -1 && io.KeyCtrl && vertices.size() >= 2)
                {
                    for (size_t i = 0; i < vertices.size(); ++i)
                    {
                        size_t next_i = (i + 1) % vertices.size();
                        ImVec2 a = GridToScreen(vertices[i]);
                        ImVec2 b = GridToScreen(vertices[next_i]);

                        ImVec2 ap = ImVec2(io.MousePos.x - a.x, io.MousePos.y - a.y);
                        ImVec2 ab = ImVec2(b.x - a.x, b.y - a.y);
                        float ab2 = ab.x * ab.x + ab.y * ab.y;
                        float t = (ab2 > 0.0f) ? (ap.x * ab.x + ap.y * ab.y) / ab2 : -1.0f;

                        if (t >= 0.0f && t <= 1.0f)
                        {
                            ImVec2 proj = ImVec2(a.x + t * ab.x, a.y + t * ab.y);
                            float dist_sq = (io.MousePos.x - proj.x) * (io.MousePos.x - proj.x) +
                                            (io.MousePos.y - proj.y) * (io.MousePos.y - proj.y);
                            if (dist_sq <= 8.0f * 8.0f)
                            {
                                glm::i32vec2 new_pt = ScreenToGrid(io.MousePos);
                                vertices.insert(vertices.begin() + static_cast<std::ptrdiff_t>(i) + 1, new_pt);
                                dragged_point_idx = static_cast<int>(i) + 1;
                                value_changed = true;
                                break;
                            }
                        }
                    }
                }

                if (dragged_point_idx == -1 && !io.KeyCtrl)
                {
                    glm::i32vec2 new_pt = ScreenToGrid(io.MousePos);
                    vertices.push_back(new_pt);
                    dragged_point_idx = static_cast<int>(vertices.size()) - 1;
                    value_changed = true;
                }
            }

            if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                for (int i = 0; i < static_cast<int>(vertices.size()); ++i)
                {
                    ImVec2 node_pos = GridToScreen(vertices[i]);
                    float dist_sq = (io.MousePos.x - node_pos.x) * (io.MousePos.x - node_pos.x) +
                                    (io.MousePos.y - node_pos.y) * (io.MousePos.y - node_pos.y);
                    if (dist_sq <= (node_radius + 4.0f) * (node_radius + 4.0f))
                    {
                        vertices.erase(vertices.begin() + i);
                        value_changed = true;
                        dragged_point_idx = -1;
                        break;
                    }
                }
            }

            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && dragged_point_idx >= 0 && dragged_point_idx < static_cast<int>(vertices.size()))
            {
                glm::i32vec2 new_grid_pos = ScreenToGrid(io.MousePos);
                if (vertices[dragged_point_idx] != new_grid_pos)
                {
                    vertices[dragged_point_idx] = new_grid_pos;
                    value_changed = true;
                }
            }

            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                dragged_point_idx = -1;

            if (vertices.size() >= 2)
            {
                for (size_t i = 0; i < vertices.size() - 1; ++i)
                {
                    ImVec2 p1 = GridToScreen(vertices[i]);
                    ImVec2 p2 = GridToScreen(vertices[i + 1]);
                    draw_list->AddLine(p1, p2, IM_COL32(0, 220, 255, 255), 2.0f);
                }
                ImVec2 p_last = GridToScreen(vertices.back());
                ImVec2 p_first = GridToScreen(vertices.front());
                draw_list->AddLine(p_last, p_first, IM_COL32(0, 220, 255, 100), 1.5f);
            }

            for (int i = 0; i < static_cast<int>(vertices.size()); ++i)
            {
                ImVec2 node_pos = GridToScreen(vertices[i]);
                ImU32 col = (i == dragged_point_idx) ? IM_COL32(255, 255, 0, 255) : IM_COL32(255, 100, 100, 255);

                draw_list->AddCircleFilled(node_pos, node_radius, col);
                draw_list->AddCircle(node_pos, node_radius, IM_COL32(0, 0, 0, 255), 0, 1.5f);

                if (i == dragged_point_idx || (is_hovered && std::abs(io.MousePos.x - node_pos.x) < 10 && std::abs(io.MousePos.y - node_pos.y) < 10))
                {
                    char buf[32];
                    std::snprintf(buf, sizeof(buf), "(%d, %d)", vertices[i].x, vertices[i].y);
                    draw_list->AddText(ImVec2(node_pos.x + 8, node_pos.y - 12), IM_COL32(255, 255, 255, 255), buf);
                }
            }

            if (!vertices.empty())
            {
                ImVec2 center_pos = GridToScreenFloat(geometry_center);
                draw_list->AddCircleFilled(center_pos, 4.0f, IM_COL32(120, 255, 120, 255));
                draw_list->AddLine(ImVec2(center_pos.x - 8.0f, center_pos.y), ImVec2(center_pos.x + 8.0f, center_pos.y), IM_COL32(120, 255, 120, 220), 1.5f);
                draw_list->AddLine(ImVec2(center_pos.x, center_pos.y - 8.0f), ImVec2(center_pos.x, center_pos.y + 8.0f), IM_COL32(120, 255, 120, 220), 1.5f);
            }
        }

        ImGui::TextDisabled(ICON_LC_MOUSE_LEFT "add/drag");
        ImGui::TextDisabled(ICON_LC_MOUSE_RIGHT "remove");
        ImGui::TextDisabled("Ctrl+" ICON_LC_MOUSE_LEFT "insert on edge");

        ImGui::PopID();
        return value_changed;
    }

    SceneTreeNodeOutput SceneTreeNode(
        const char* str_id,
        SceneTreeNodeFlags flags,
        bool is_open,
        const char* icon,
        std::string& name)
    {
        SceneTreeNodeOutput out;

        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return out;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(str_id);

        const float row_height = ImGui::GetFrameHeight();
        const float eye_width = 20.0f;
        const float icon_width = 20.0f;
        const float EYE_SPACING = 6.0f; 

        const ImVec2 pos = window->DC.CursorPos;
        const float avail_width = ImGui::GetContentRegionAvail().x;
        const ImVec2 size(avail_width, row_height);

        const ImRect total_bb(pos, ImVec2(pos.x + size.x, pos.y + row_height));
        ImGui::ItemSize(total_bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(total_bb, id))
            return out;

        const float icon_start_x = pos.x + eye_width + EYE_SPACING;
        const float text_start_x = icon_start_x + (icon && icon[0] != '\0' ? icon_width : 0.0f);

        const ImRect eye_bb(pos, ImVec2(pos.x + eye_width, pos.y + row_height));
        const ImRect icon_bb(ImVec2(icon_start_x, pos.y), ImVec2(icon_start_x + icon_width, pos.y + row_height));
        const ImRect text_bb(ImVec2(text_start_x, pos.y), ImVec2(pos.x + size.x, pos.y + row_height));
        
        const ImRect full_row_bb(pos, ImVec2(pos.x + size.x, pos.y + row_height));
        bool is_hovered = ImGui::IsMouseHoveringRect(full_row_bb.Min, full_row_bb.Max);

        if (ImGui::IsMouseHoveringRect(eye_bb.Min, eye_bb.Max) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            out.visibility_toggled = true;

        if (flags.is_group && ImGui::IsMouseHoveringRect(icon_bb.Min, icon_bb.Max) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            out.open_toggled = true;

        ImDrawList* draw_list = window->DrawList;

        if (flags.is_selected)
            draw_list->AddRectFilled(full_row_bb.Min, full_row_bb.Max, ImGui::GetColorU32(ImGuiCol_Header));
        else if (is_hovered)
            draw_list->AddRectFilled(full_row_bb.Min, full_row_bb.Max, ImGui::GetColorU32(ImGuiCol_HeaderHovered));

        ImU32 eye_col = flags.visible ? ImGui::GetColorU32(ImGuiCol_Text) : ImGui::GetColorU32(ImGuiCol_TextDisabled);
        const char* eye_icon = flags.visible ? ICON_LC_EYE : ICON_LC_EYE_OFF;
        ImVec2 eye_pos(eye_bb.Min.x + (eye_width - ImGui::CalcTextSize(eye_icon).x) * 0.5f, eye_bb.Min.y + (row_height - ImGui::GetFontSize()) * 0.5f);
        draw_list->AddText(eye_pos, eye_col, eye_icon);

        if (icon && icon[0] != '\0')
        {
            ImVec2 icon_pos(icon_start_x + (icon_width - ImGui::CalcTextSize(icon).x) * 0.5f, pos.y + (row_height - ImGui::GetFontSize()) * 0.5f);
            draw_list->AddText(icon_pos, ImGui::GetColorU32(ImGuiCol_Text), icon);
        }

        if (flags.is_renaming)
        {
            float input_w = full_row_bb.Max.x - text_start_x - 4.0f;

            ImGui::SetCursorScreenPos(ImVec2(text_start_x, pos.y + (row_height - ImGui::GetFontSize()) * 0.5f - style.FramePadding.y));
            ImGui::SetNextItemWidth(input_w);

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetColorU32(ImGuiCol_FrameBg));

            ImGui::PushID("rename_field");
            
            ImGuiID focus_id = ImGui::GetID("rename_focus");
            bool* p_focused = ImGui::GetStateStorage()->GetBoolRef(focus_id, false);
            if (!*p_focused)
            {
                ImGui::SetKeyboardFocusHere(0);
                *p_focused = true;
            }

            if (ImGui::InputText("##NameInput", &name, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
                out.rename_submitted = true;
            if (ImGui::IsItemDeactivated())
                out.rename_submitted = true;

            ImGui::PopID();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }
        else
        {
            ImGui::PushID("rename_field");
            ImGui::GetStateStorage()->SetBool(ImGui::GetID("rename_focus"), false);
            ImGui::PopID();

            ImGui::SetCursorScreenPos(full_row_bb.Min);
            ImGui::InvisibleButton(str_id, full_row_bb.GetSize());
            
            if (ImGui::IsItemHovered())
            {
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) 
                    out.right_clicked = true;

                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsMouseHoveringRect(text_bb.Min, text_bb.Max)) 
                    out.name_double_clicked = true;
            }
            
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsMouseHoveringRect(eye_bb.Min, eye_bb.Max)) 
                out.clicked = true;

            ImVec2 text_pos(text_start_x, pos.y + (row_height - ImGui::GetFontSize()) * 0.5f);
            draw_list->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), name.c_str());
        }

        return out;
    }
}