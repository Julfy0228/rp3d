#include "SceneObjectsPanel.h"

#include <algorithm>
#include <cfloat>
#include <cstddef>
#include <memory>

#include <imgui.h>
#include <imgui_stdlib.h>

void SceneObjectsPanel::clear_selection(SceneNode* node)
{
    if (!node) return;

    node->selected = false;
    if (!node->is_group()) return;

    Group* group = static_cast<Group*>(node);
    for (auto& child : group->children)
        clear_selection(child.get());
}

void SceneObjectsPanel::clear_selection(Scene& scene)
{
    for (auto& child : scene.root.children)
        clear_selection(child.get());
}

void SceneObjectsPanel::collect_selected_nodes(SceneNode* node, std::vector<SceneNode*>& selected_nodes)
{
    if (!node) return;

    if (node->selected)
        selected_nodes.push_back(node);

    if (!node->is_group()) return;

    Group* group = static_cast<Group*>(node);
    for (auto& child : group->children)
        collect_selected_nodes(child.get(), selected_nodes);
}

void SceneObjectsPanel::select_single_node(Scene& scene, SceneNode* node)
{
    clear_selection(scene);
    if (node)
    {
        node->selected = true;
    }
}

Group* SceneObjectsPanel::get_selected_group(Scene& scene) const
{
    std::vector<SceneNode*> selected_nodes;
    for (auto& child : scene.root.children)
        const_cast<SceneObjectsPanel*>(this)->collect_selected_nodes(child.get(), selected_nodes);

    if (selected_nodes.size() != 1)
        return nullptr;

    SceneNode* node = selected_nodes.front();
    if (!node->is_group())
        return nullptr;

    return static_cast<Group*>(node);
}

bool SceneObjectsPanel::find_parent_group(Group* current, SceneNode* target, Group*& parent_group, size_t& child_index)
{
    if (!current || !target) return false;

    for (size_t i = 0; i < current->children.size(); ++i)
        if (current->children[i].get() == target)
        {
            parent_group = current;
            child_index = i;
            return true;
        }

    for (auto& child : current->children)
    {
        if (!child || !child->is_group())
            continue;

        if (find_parent_group(static_cast<Group*>(child.get()), target, parent_group, child_index))
            return true;
    }

    return false;
}

bool SceneObjectsPanel::is_descendant_of(const Group* candidate_parent, const SceneNode* node) const
{
    if (!candidate_parent || !node)
        return false;

    for (const auto& child : candidate_parent->children) {
        if (!child) continue;

        if (child.get() == node) return true;

        if (child->is_group() && is_descendant_of(static_cast<const Group*>(child.get()), node))
            return true;
    }

    return false;
}

bool SceneObjectsPanel::can_drop_node(SceneNode* node, Group* new_parent) const
{
    if (!node || !new_parent || node == new_parent) return false;

    if (node->is_group() && is_descendant_of(static_cast<const Group*>(node), new_parent))
        return false;

    return true;
}

bool SceneObjectsPanel::move_node(Scene& scene, SceneNode* node, Group* new_parent, size_t insert_index)
{
    if (!can_drop_node(node, new_parent)) return false;

    Group* old_parent = nullptr;
    size_t old_index = 0;
    if (!find_parent_group(&scene.root, node, old_parent, old_index))
        return false;

    std::shared_ptr<SceneNode> detached_node = old_parent->children[old_index];
    old_parent->children.erase(old_parent->children.begin() + static_cast<std::ptrdiff_t>(old_index));

    if (old_parent == new_parent && old_index < insert_index)
        insert_index -= 1;

    insert_index = std::min(insert_index, new_parent->children.size());
    new_parent->children.insert(new_parent->children.begin() + static_cast<std::ptrdiff_t>(insert_index), detached_node);
    return true;
}

bool SceneObjectsPanel::reparent_node(Scene& scene, SceneNode* node, Group* new_parent)
{
    return move_node(scene, node, new_parent, new_parent->children.size());
}

void SceneObjectsPanel::handle_root_insert_target(Scene& scene, size_t insert_index)
{
    ImGui::PushID(static_cast<int>(insert_index));
    ImVec2 line_start = ImGui::GetCursorScreenPos();
    ImVec2 line_size(std::max(1.0f, ImGui::GetContentRegionAvail().x), 6.0f);
    ImGui::InvisibleButton("##RootInsertTarget", line_size);

    if (ImGui::IsItemHovered())
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        float y = line_start.y + line_size.y * 0.5f;
        draw_list->AddLine(ImVec2(line_start.x, y), ImVec2(line_start.x + line_size.x, y), IM_COL32(120, 200, 255, 255), 2.0f);
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_NODE"))
        {
            SceneNode* payload_node = *static_cast<SceneNode* const*>(payload->Data);
            move_node(scene, payload_node, &scene.root, insert_index);
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::PopID();
}

void SceneObjectsPanel::handle_scene_drop_target(Scene& scene, SceneNode* target_node, Group* target_parent, size_t target_index, bool allow_inside_drop)
{
    if (!target_node || !target_parent) return;

    ImVec2 item_min = ImGui::GetItemRectMin();
    ImVec2 item_max = ImGui::GetItemRectMax();
    ImVec2 mouse_pos = ImGui::GetIO().MousePos;
    float item_height = item_max.y - item_min.y;
    float edge_band = std::min(6.0f, item_height * 0.25f);
    bool before_zone = mouse_pos.y <= item_min.y + edge_band;
    bool after_zone = mouse_pos.y >= item_max.y - edge_band;

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_NODE", ImGuiDragDropFlags_AcceptBeforeDelivery)) {
            SceneNode* payload_node = *static_cast<SceneNode* const*>(payload->Data);
            Group* inside_group = allow_inside_drop && target_node->is_group() ? static_cast<Group*>(target_node) : nullptr;
            SceneDropMode drop_mode = SceneDropMode::After;
            Group* destination_parent = target_parent;
            size_t insert_index = target_index + 1;

            if (before_zone)
            {
                drop_mode = SceneDropMode::Before;
                destination_parent = target_parent;
                insert_index = target_index;
            }
            else if (after_zone)
            {
                drop_mode = SceneDropMode::After;
                destination_parent = target_parent;
                insert_index = target_index + 1;
            }
            else if (inside_group)
            {
                drop_mode = SceneDropMode::Inside;
                destination_parent = inside_group;
                insert_index = inside_group->children.size();
            }

            if (ImGui::IsMouseHoveringRect(item_min, item_max))
            {
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                if (drop_mode == SceneDropMode::Before)
                {
                    draw_list->AddLine(item_min, ImVec2(item_max.x, item_min.y), IM_COL32(120, 200, 255, 255), 2.0f);
                }
                else if (drop_mode == SceneDropMode::After)
                {
                    draw_list->AddLine(ImVec2(item_min.x, item_max.y), item_max, IM_COL32(120, 200, 255, 255), 2.0f);
                }
                else
                {
                    draw_list->AddRect(item_min, item_max, IM_COL32(120, 200, 255, 255), 0.0f, 0, 2.0f);
                }
            }

            if (payload->IsDelivery() && can_drop_node(payload_node, destination_parent))
                move_node(scene, payload_node, destination_parent, insert_index);
        }
        ImGui::EndDragDropTarget();
    }
}

void SceneObjectsPanel::draw_scene_node_row(Scene& scene, SceneNode* node, int depth)
{
    if (!node) return;

    ImGui::PushID(node->id);

    if (depth > 0)
        ImGui::Indent(static_cast<float>(depth) * 16.0f);

    bool visible = node->visible;
    if (ImGui::Checkbox("##Visible", &visible))
        node->visible = visible;

    ImGui::SameLine();

    bool opened = false;

    if (node->is_group())
    {
        ImGuiTreeNodeFlags tree_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (node->selected)
            tree_flags |= ImGuiTreeNodeFlags_Selected;

        if (rename_node_id == node->id)
        {
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputText("##Rename", &node->name, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
                rename_node_id = -1;
            if (ImGui::IsItemDeactivated())
                rename_node_id = -1;
            if (ImGui::IsItemActivated())
                ImGui::SetKeyboardFocusHere(-1);
        }
        else
        {
            opened = ImGui::TreeNodeEx("##Node", tree_flags, "%s", node->name.c_str());
            if (ImGui::IsItemClicked())
                select_single_node(scene, node);

            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered())
            {
                rename_node_id = node->id;
                select_single_node(scene, node);
            }
        }
    }
    else
    {
        ImGuiTreeNodeFlags tree_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet;
        if (node->selected)
            tree_flags |= ImGuiTreeNodeFlags_Selected;

        if (rename_node_id == node->id)
        {
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputText("##Rename", &node->name, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
                rename_node_id = -1;
            if (ImGui::IsItemDeactivated())
                rename_node_id = -1;
            if (ImGui::IsItemActivated())
                ImGui::SetKeyboardFocusHere(-1);
        }
        else
        {
            ImGui::TreeNodeEx("##Node", tree_flags, "%s", node->name.c_str());
            if (ImGui::IsItemClicked())
                select_single_node(scene, node);

            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered())
            {
                rename_node_id = node->id;
                select_single_node(scene, node);
            }
        }
    }

    if (ImGui::BeginDragDropSource())
    {
        SceneNode* payload_node = node;
        ImGui::SetDragDropPayload("SCENE_NODE", &payload_node, sizeof(payload_node));
        ImGui::TextUnformatted(node->name.c_str());
        ImGui::EndDragDropSource();
    }

    if (node->is_group() && ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_NODE"))
        {
            SceneNode* payload_node = *static_cast<SceneNode* const*>(payload->Data);
            reparent_node(scene, payload_node, static_cast<Group*>(node));
        }
        ImGui::EndDragDropTarget();
    }

    if (depth > 0)
        ImGui::Unindent(static_cast<float>(depth) * 16.0f);

    if (opened)
    {
        Group* group = static_cast<Group*>(node);
        for (auto& child : group->children)
            draw_scene_node_row(scene, child.get(), depth + 1);
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void SceneObjectsPanel::draw(Scene& scene)
{
    ImGui::Begin("Objects");

    if (ImGui::Button("Add Item"))
    {
        Group* parent = get_selected_group(scene);
        std::shared_ptr<Item> item = scene.create_node<Item>(parent);
        if (item->name.empty())
            item->name = "Item";
        select_single_node(scene, item.get());
    }

    ImGui::SameLine();

    if (ImGui::Button("Add Group"))
    {
        Group* parent = get_selected_group(scene);
        std::shared_ptr<Group> group = scene.create_node<Group>(parent);
        if (group->name.empty())
            group->name = "Group";
        select_single_node(scene, group.get());
    }

    handle_root_insert_target(scene, 0);
    for (size_t i = 0; i < scene.root.children.size(); ++i)
    {
        SceneNode* node = scene.root.children[i].get();
        draw_scene_node_row(scene, node, 0);
        handle_scene_drop_target(scene, node, &scene.root, i, node->is_group());
    }
    handle_root_insert_target(scene, scene.root.children.size());

    ImGui::End();
}