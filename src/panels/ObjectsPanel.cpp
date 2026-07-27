#include "ObjectsPanel.h"
#include "Widgets.h"

#include "../UndoManager.h"

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_set>

#include <imgui.h>
#include <imgui_internal.h>
#include <IconsLucide.h>
#include <Colors.h>

std::string ObjectsPanel::generate_unique_name(const Scene& scene, const std::string& prefix) const
{
    std::unordered_set<std::string> existing_names;

    auto collect_names = [&](auto& self, const Group& group) -> void {
        for (const auto& child : group.children)
        {
            if (child)
            {
                existing_names.insert(child->name);
                if (child->is_group())
                    self(self, *static_cast<const Group*>(child.get()));
            }
        }
    };

    collect_names(collect_names, scene.root);

    int index = 0;
    while (true)
    {
        std::string candidate = prefix + std::to_string(index);
        if (existing_names.find(candidate) == existing_names.end())
            return candidate;
        index++;
    }
}

std::string ObjectsPanel::generate_unique_copy_name(const Scene& scene, const std::string& base_name) const
{
    std::unordered_set<std::string> existing_names;

    auto collect_names = [&](auto& self, const Group& group) -> void {
        for (const auto& child : group.children)
        {
            if (!child)
                continue;

            existing_names.insert(child->name);
            if (child->is_group())
                self(self, *static_cast<const Group*>(child.get()));
        }
    };

    collect_names(collect_names, scene.root);

    if (existing_names.find(base_name) == existing_names.end())
        return base_name;

    int index = 0;
    while (true)
    {
        std::string candidate = base_name + " #" + std::to_string(index);
        if (existing_names.find(candidate) == existing_names.end())
            return candidate;
        index++;
    }
}

void ObjectsPanel::clear_selection(SceneNode* node)
{
    if (!node) return;
    node->selected = false;
    if (node->is_group())
    {
        auto* group = static_cast<Group*>(node);
        for (auto& child : group->children)
            clear_selection(child.get());
    }
}

void ObjectsPanel::clear_descendant_selection(SceneNode* node)
{
    if (!node || !node->is_group())
        return;

    auto* group = static_cast<Group*>(node);
    for (auto& child : group->children)
    {
        clear_selection(child.get());
    }
}

void ObjectsPanel::collect_selected_roots(Group& group, std::vector<SceneNode*>& roots, bool ancestor_selected) const
{
    for (auto& child : group.children)
    {
        if (!child)
            continue;

        const bool is_root = child->selected && !ancestor_selected;
        if (is_root)
            roots.push_back(child.get());

        if (child->is_group())
            collect_selected_roots(*static_cast<Group*>(child.get()), roots, ancestor_selected || child->selected);
    }
}

void ObjectsPanel::collect_selected_root_parents(Group& root, const std::vector<SceneNode*>& selected_roots, std::vector<Group*>& parents) const
{
    parents.reserve(selected_roots.size());
    for (SceneNode* node : selected_roots)
        parents.push_back(find_parent(root, node));
}

ObjectsPanel::ClipboardNode ObjectsPanel::make_clipboard_node(const SceneNode& node) const
{
    ClipboardNode clipboard_node;
    clipboard_node.is_group = node.is_group();
    clipboard_node.name = node.name;
    clipboard_node.visible = node.visible;
    clipboard_node.position = node.position;
    clipboard_node.rotation = node.rotation;

    if (node.is_group())
    {
        const auto& group = static_cast<const Group&>(node);
        clipboard_node.children.reserve(group.children.size());
        for (const auto& child : group.children)
        {
            if (child)
                clipboard_node.children.push_back(make_clipboard_node(*child));
        }
    }
    else
    {
        const auto& item = static_cast<const Item&>(node);
        clipboard_node.vertices = item.vertices;
        clipboard_node.thickness = item.thickness;
        clipboard_node.color = item.color;
    }

    return clipboard_node;
}

std::shared_ptr<SceneNode> ObjectsPanel::clone_from_clipboard(Scene& scene, Group& parent, const ClipboardNode& clipboard_node)
{
    if (clipboard_node.is_group)
    {
        auto group = scene.create_node<Group>(&parent);
        group->name = generate_unique_copy_name(scene, clipboard_node.name);
        group->visible = clipboard_node.visible;
        group->selected = false;
        group->position = clipboard_node.position;
        group->rotation = clipboard_node.rotation;
        for (const ClipboardNode& child : clipboard_node.children)
            clone_from_clipboard(scene, *group, child);
        return group;
    }

    auto item = scene.create_node<Item>(&parent);
    item->name = generate_unique_copy_name(scene, clipboard_node.name);
    item->visible = clipboard_node.visible;
    item->selected = false;
    item->position = clipboard_node.position;
    item->rotation = clipboard_node.rotation;
    item->vertices = clipboard_node.vertices;
    item->thickness = clipboard_node.thickness;
    item->color = clipboard_node.color;
    return item;
}

bool ObjectsPanel::remove_node(Group& root, SceneNode* node)
{
    for (auto it = root.children.begin(); it != root.children.end(); ++it)
    {
        if (!*it)
            continue;

        if (it->get() == node)
        {
            root.children.erase(it);
            return true;
        }

        if ((*it)->is_group() && remove_node(*static_cast<Group*>(it->get()), node))
            return true;
    }

    return false;
}

void ObjectsPanel::delete_selected(Scene& scene)
{
    std::vector<SceneNode*> selected_roots;
    collect_selected_roots(scene.root, selected_roots);
    if (selected_roots.empty())
        return;

    if (undo_manager)
        undo_manager->capture_snapshot(scene);

    rename_node_id = -1;
    for (SceneNode* node : selected_roots)
        remove_node(scene.root, node);
}

void ObjectsPanel::copy_selected(Scene& scene)
{
    std::vector<SceneNode*> selected_roots;
    collect_selected_roots(scene.root, selected_roots);
    clipboard_nodes.clear();
    clipboard_nodes.reserve(selected_roots.size());
    for (SceneNode* node : selected_roots)
        clipboard_nodes.push_back(make_clipboard_node(*node));
    clipboard_is_cut = false;
}

void ObjectsPanel::cut_selected(Scene& scene)
{
    std::vector<SceneNode*> selected_roots;
    collect_selected_roots(scene.root, selected_roots);
    clipboard_nodes.clear();
    clipboard_nodes.reserve(selected_roots.size());
    for (SceneNode* node : selected_roots)
        clipboard_nodes.push_back(make_clipboard_node(*node));

    clipboard_is_cut = !clipboard_nodes.empty();
    if (clipboard_is_cut)
        delete_selected(scene);
}

void ObjectsPanel::paste_selected(Scene& scene)
{
    if (clipboard_nodes.empty())
        return;

    if (undo_manager)
        undo_manager->capture_snapshot(scene);

    std::vector<SceneNode*> selected_roots;
    collect_selected_roots(scene.root, selected_roots);

    Group* target_parent = &scene.root;
    if (!selected_roots.empty())
    {
        Group* common_parent = find_parent(scene.root, selected_roots.front());
        bool same_parent = common_parent != nullptr;
        for (size_t i = 1; i < selected_roots.size(); ++i)
        {
            if (find_parent(scene.root, selected_roots[i]) != common_parent)
            {
                same_parent = false;
                break;
            }
        }

        if (same_parent && common_parent)
            target_parent = common_parent;
    }

    clear_selection(&scene.root);
    std::vector<std::shared_ptr<SceneNode>> pasted_nodes;
    pasted_nodes.reserve(clipboard_nodes.size());
    for (const ClipboardNode& clipboard_node : clipboard_nodes)
        pasted_nodes.push_back(clone_from_clipboard(scene, *target_parent, clipboard_node));

    for (const auto& node : pasted_nodes)
    {
        if (node)
            node->selected = true;
    }

    rename_node_id = -1;
    if (clipboard_is_cut)
    {
        clipboard_nodes.clear();
        clipboard_is_cut = false;
    }
}

void ObjectsPanel::handle_shortcuts(Scene& scene)
{
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
        return;

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput)
        return;

    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
    {
        delete_selected(scene);
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_F2, false))
    {
        std::vector<SceneNode*> selected_roots;
        collect_selected_roots(scene.root, selected_roots);
        if (selected_roots.size() == 1)
            rename_node_id = selected_roots.front()->id;
        return;
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C, false))
    {
        copy_selected(scene);
        return;
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_X, false))
    {
        cut_selected(scene);
        return;
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V, false))
    {
        paste_selected(scene);
        return;
    }
}

Group* ObjectsPanel::find_parent(Group& root, const SceneNode* node) const
{
    for (auto& child : root.children)
    {
        if (!child)
            continue;

        if (child.get() == node)
            return &root;

        if (child->is_group())
        {
            if (Group* parent = find_parent(*static_cast<Group*>(child.get()), node))
                return parent;
        }
    }

    return nullptr;
}

bool ObjectsPanel::selection_has_same_parent(const Group& root, const SceneNode* node) const
{
    Group* node_parent = find_parent(const_cast<Group&>(root), node);
    bool has_selection = false;

    auto validate = [&](auto& self, const Group& group) -> bool {
        for (const auto& child : group.children)
        {
            if (!child)
                continue;

            if (child->selected)
            {
                has_selection = true;
                Group* selected_parent = find_parent(const_cast<Group&>(root), child.get());
                if (selected_parent != node_parent)
                    return false;
            }

            if (child->is_group() && !self(self, *static_cast<const Group*>(child.get())))
                return false;
        }

        return true;
    };

    const bool valid = validate(validate, root);
    return !has_selection || valid;
}

bool ObjectsPanel::has_selected_ancestor(const Group& group, const SceneNode* node) const
{
    for (const auto& child : group.children)
    {
        if (!child)
            continue;

        if (child.get() == node)
            return false;

        if (child->is_group())
        {
            auto* child_group = static_cast<const Group*>(child.get());
            if (is_descendant_of(child_group, node))
                return child->selected || has_selected_ancestor(*child_group, node);
        }
    }

    return false;
}

bool ObjectsPanel::is_descendant_of(const Group* candidate, const SceneNode* node) const
{
    if (!candidate || !node) return false;
    for (const auto& child : candidate->children)
    {
        if (child.get() == node) return true;
        if (child->is_group() && is_descendant_of(static_cast<const Group*>(child.get()), node))
            return true;
    }
    return false;
}

bool ObjectsPanel::reorder_node(Scene& scene, SceneNode* node, Group* target_parent, size_t target_index)
{
    if (!node || !target_parent) return false;
    if (node == target_parent || (node->is_group() && is_descendant_of(static_cast<const Group*>(node), target_parent)))
        return false;

    std::shared_ptr<SceneNode> detached;
    
    auto extract = [&](auto& self, Group& current) -> bool {
        for (auto it = current.children.begin(); it != current.children.end(); ++it)
        {
            if (it->get() == node)
            {
                if (&current == target_parent)
                {
                    size_t old_index = std::distance(current.children.begin(), it);
                    if (old_index < target_index)
                    {
                        target_index--;
                    }
                }

                detached = std::move(*it);
                current.children.erase(it);
                return true;
            }
            if ((*it)->is_group() && self(self, *static_cast<Group*>(it->get())))
                return true;
        }
        return false;
    };

    if (extract(extract, scene.root) && detached)
    {
        if (target_index > target_parent->children.size())
            target_index = target_parent->children.size();

        target_parent->children.insert(target_parent->children.begin() + target_index, std::move(detached));
        return true;
    }

    return false;
}

bool ObjectsPanel::reorder_selected_nodes(Scene& scene, SceneNode* dragged_node, Group* target_parent, size_t target_index)
{
    if (!dragged_node || !target_parent)
        return false;

    std::vector<SceneNode*> selected_roots;
    collect_selected_roots(scene.root, selected_roots);
    if (selected_roots.empty())
        return false;

    if (undo_manager)
        undo_manager->capture_snapshot(scene);

    const bool dragged_is_selected = std::find(selected_roots.begin(), selected_roots.end(), dragged_node) != selected_roots.end();
    if (!dragged_is_selected)
    {
        const bool reordered = reorder_node(scene, dragged_node, target_parent, target_index);
        if (!reordered && undo_manager && undo_manager->can_undo())
            undo_manager->undo(scene);
        return reordered;
    }

    if (std::find(selected_roots.begin(), selected_roots.end(), static_cast<SceneNode*>(target_parent)) != selected_roots.end())
    {
        if (undo_manager && undo_manager->can_undo())
            undo_manager->undo(scene);
        return false;
    }

    for (SceneNode* node : selected_roots)
    {
        if (node->is_group() && is_descendant_of(static_cast<const Group*>(node), target_parent))
        {
            if (undo_manager && undo_manager->can_undo())
                undo_manager->undo(scene);
            return false;
        }
    }

    std::vector<Group*> original_parents;
    collect_selected_root_parents(scene.root, selected_roots, original_parents);

    std::vector<std::shared_ptr<SceneNode>> detached_nodes;
    detached_nodes.reserve(selected_roots.size());

    size_t adjusted_target_index = target_index;
    for (size_t i = 0; i < selected_roots.size(); ++i)
    {
        Group* original_parent = original_parents[i];
        if (original_parent != target_parent)
            continue;

        auto it = std::find_if(
            original_parent->children.begin(),
            original_parent->children.end(),
            [node = selected_roots[i]](const std::shared_ptr<SceneNode>& child) {
                return child.get() == node;
            });
        if (it == original_parent->children.end())
            continue;

        size_t old_index = static_cast<size_t>(std::distance(original_parent->children.begin(), it));
        if (old_index < adjusted_target_index)
            adjusted_target_index--;
    }

    for (SceneNode* node : selected_roots)
    {
        std::shared_ptr<SceneNode> detached;
        auto extract = [&](auto& self, Group& current) -> bool {
            for (auto it = current.children.begin(); it != current.children.end(); ++it)
            {
                if (!*it)
                    continue;

                if (it->get() == node)
                {
                    detached = std::move(*it);
                    current.children.erase(it);
                    return true;
                }

                if ((*it)->is_group() && self(self, *static_cast<Group*>(it->get())))
                    return true;
            }

            return false;
        };

        if (extract(extract, scene.root) && detached)
            detached_nodes.push_back(std::move(detached));
    }

    if (detached_nodes.empty())
    {
        if (undo_manager && undo_manager->can_undo())
            undo_manager->undo(scene);
        return false;
    }

    if (adjusted_target_index > target_parent->children.size())
        adjusted_target_index = target_parent->children.size();

    target_parent->children.insert(
        target_parent->children.begin() + static_cast<std::ptrdiff_t>(adjusted_target_index),
        std::make_move_iterator(detached_nodes.begin()),
        std::make_move_iterator(detached_nodes.end()));

    return true;
}

void ObjectsPanel::draw_context_menu(Scene& scene, SceneNode* node, Group* parent, const char* popup_id)
{
    if (ImGui::BeginPopup(popup_id))
    {
        if (node && !node->selected)
        {
            if (rename_node_id != -1 && rename_node_id != node->id)
                rename_node_id = -1;

            clear_selection(&scene.root);
            node->selected = true;
        }

        Group* target_group = nullptr;
        if (node && node->is_group())
            target_group = static_cast<Group*>(node);
        else
            target_group = parent ? parent : &scene.root;

        if (ImGui::MenuItem(ICON_LC_FILE_PLUS "Add cube"))
        {
            if (undo_manager)
                undo_manager->capture_snapshot(scene);

            auto item = scene.create_node<Item>(target_group);
            item->name = generate_unique_name(scene, "Cube");
            clear_selection(&scene.root);
            item->selected = true;
            rename_node_id = item->id;
        }

        if (ImGui::MenuItem(ICON_LC_FOLDER_PLUS "Add group"))
        {
            if (undo_manager)
                undo_manager->capture_snapshot(scene);

            auto group = scene.create_node<Group>(target_group);
            group->name = generate_unique_name(scene, "Group");
            clear_selection(&scene.root);
            group->selected = true;
            rename_node_id = group->id;
        }

        if (node)
        {
            ImGui::Separator();
            if (ImGui::MenuItem(ICON_LC_PENCIL "Rename", "F2"))
            {
                rename_node_id = node->id;
            }

            const bool has_selection = node->selected;
            if (ImGui::MenuItem(ICON_LC_COPY "Copy", "Ctrl+C", false, has_selection))
            {
                copy_selected(scene);
            }
            if (ImGui::MenuItem(ICON_LC_SCISSORS "Cut", "Ctrl+X", false, has_selection))
            {
                cut_selected(scene);
            }
            if (ImGui::MenuItem(ICON_LC_CLIPBOARD_PASTE "Paste", "Ctrl+V", false, !clipboard_nodes.empty()))
            {
                paste_selected(scene);
            }

            ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDanger);
            if (ImGui::MenuItem(ICON_LC_TRASH_2 "Delete", "Del"))
            {
                delete_selected(scene);
            }
            ImGui::PopStyleColor();
        }

        ImGui::EndPopup();
    }
}

void ObjectsPanel::draw_node(Scene& scene, SceneNode* node, Group* parent, size_t index_in_parent)
{
    if (!node) return;

    ImGui::PushID(node);

    const bool is_group = node->is_group();

    ImGuiStorage* storage = ImGui::GetStateStorage();
    ImGuiID open_state_id = ImGui::GetID("##node_open_state");
    bool is_open = storage->GetBool(open_state_id, false);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2.0f);
    ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 4.0f));
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_NODE"))
        {
            SceneNode* dropped_node = *static_cast<SceneNode**>(payload->Data);
            drag_reorder_committed = reorder_selected_nodes(scene, dropped_node, parent, index_in_parent) || drag_reorder_committed;
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2.0f);

    Widgets::SceneTreeNodeFlags flags;
    flags.is_group = is_group;
    flags.is_selected = node->selected;
    flags.visible = node->visible;
    flags.is_renaming = (rename_node_id == node->id);

    const char* icon = is_group ? (is_open ? ICON_LC_FOLDER_OPEN : ICON_LC_FOLDER) : ICON_LC_FILE;

    auto res = Widgets::SceneTreeNode("##tree_node", flags, is_open, icon, node->name);

    if (is_group)
    {
        const Group* group = static_cast<const Group*>(node);
        const std::string child_count_text = std::to_string(group->children.size());
        const ImVec2 text_size = ImGui::CalcTextSize(child_count_text.c_str());
        const ImVec2 item_min = ImGui::GetItemRectMin();
        const ImVec2 item_max = ImGui::GetItemRectMax();
        const float text_x = item_max.x - text_size.x - 8.0f;
        const float text_y = item_min.y + (item_max.y - item_min.y - text_size.y) * 0.5f;

        ImGui::GetWindowDrawList()->AddText(
            ImVec2(text_x, text_y),
            ImGui::GetColorU32(ImGuiCol_TextDisabled),
            child_count_text.c_str());
    }

    if (res.visibility_toggled)
    {
        node->visible = !node->visible;
    }

    if (res.open_toggled && is_group)
    {
        is_open = !is_open;
        storage->SetBool(open_state_id, is_open);
    }

    if (res.clicked)
    {
        if (rename_node_id != -1 && rename_node_id != node->id)
            rename_node_id = -1;

        ImGuiIO& io = ImGui::GetIO();

        if (io.KeyCtrl)
        {
            pending_single_select_node_id = -1;
            if (!node->selected)
            {
                if (!has_selected_ancestor(scene.root, node) && selection_has_same_parent(scene.root, node))
                {
                    node->selected = true;
                    clear_descendant_selection(node);
                }
            }
            else
                node->selected = false;
        }
        else
        {
            if (!node->selected)
            {
                pending_single_select_node_id = -1;
                clear_selection(&scene.root);
                node->selected = true;
            }
            else
            {
                pending_single_select_node_id = node->id;
            }
        }
    }

    if (res.right_clicked)
    {
        if (rename_node_id != -1 && rename_node_id != node->id)
            rename_node_id = -1;

        if (!node->selected)
        {
            pending_single_select_node_id = -1;
            clear_selection(&scene.root);
            node->selected = true;
        }

        ImGui::OpenPopup("ObjectsContextMenu");
    }

    if (res.name_double_clicked)
    {
        rename_node_id = node->id;
    }

    if (res.rename_submitted)
    {
        rename_node_id = -1;
        if (node->name.empty())
        {
            std::string prefix = is_group ? "Group" : "Cube";
            node->name = generate_unique_name(scene, prefix);
        }
    }

    draw_context_menu(scene, node, parent, "ObjectsContextMenu");

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        SceneNode* payload = node;
        ImGui::SetDragDropPayload("SCENE_NODE", &payload, sizeof(SceneNode*));
        ImGui::Text("%s %s", icon, node->name.c_str());
        ImGui::EndDragDropSource();
    }

    if (is_group && ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_NODE"))
        {
            SceneNode* dropped_node = *static_cast<SceneNode**>(payload->Data);
            Group* group_node = static_cast<Group*>(node);
            
            drag_reorder_committed = reorder_selected_nodes(scene, dropped_node, group_node, group_node->children.size()) || drag_reorder_committed;
            storage->SetBool(open_state_id, true);
        }
        ImGui::EndDragDropTarget();
    }

    if (is_group && is_open)
    {
        ImGui::Indent(16.0f);
        auto* group = static_cast<Group*>(node);
        auto children_copy = group->children;
        for (size_t i = 0; i < children_copy.size(); ++i)
        {
            draw_node(scene, children_copy[i].get(), group, i);
        }
        ImGui::Unindent(16.0f);
    }

    ImGui::PopID();
}

void ObjectsPanel::draw(Scene& scene, UndoManager* undo_manager_instance)
{
    undo_manager = undo_manager_instance;
    ImGui::Begin(ICON_LC_BOXES "Objects###ObjectsPanel");

    handle_shortcuts(scene);

    auto root_children = scene.root.children;
    for (size_t i = 0; i < root_children.size(); ++i)
    {
        draw_node(scene, root_children[i].get(), &scene.root, i);
    }

    ImGui::Spacing();
    const float drop_gap_height = 8.0f;
    ImVec2 empty_area_size = ImGui::GetContentRegionAvail();
    empty_area_size.x = std::max(empty_area_size.x, 1.0f);
    empty_area_size.y = std::max(empty_area_size.y, drop_gap_height);
    ImGui::InvisibleButton("##bottom_empty_area", empty_area_size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        clear_selection(&scene.root);
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        clear_selection(&scene.root);
        ImGui::OpenPopup("ObjectsContextMenu");
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_NODE"))
        {
            SceneNode* dropped_node = *static_cast<SceneNode**>(payload->Data);
            drag_reorder_committed = reorder_selected_nodes(scene, dropped_node, &scene.root, scene.root.children.size()) || drag_reorder_committed;
        }
        ImGui::EndDragDropTarget();
    }

    draw_context_menu(scene, nullptr, &scene.root, "ObjectsContextMenu");

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        if (pending_single_select_node_id != -1 && !drag_reorder_committed)
        {
            std::vector<SceneNode*> selected_roots;
            collect_selected_roots(scene.root, selected_roots);
            for (SceneNode* selected_node : selected_roots)
            {
                if (selected_node && selected_node->id != pending_single_select_node_id)
                    selected_node->selected = false;
            }
        }

        pending_single_select_node_id = -1;
        drag_reorder_committed = false;
    }

    ImGui::End();
}