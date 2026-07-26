#include "ObjectsPanel.h"
#include "Widgets.h"

#include <algorithm>
#include <memory>
#include <unordered_set>
#include <string>

#include <imgui.h>
#include <imgui_internal.h>
#include <IconsLucide.h>

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

void ObjectsPanel::draw_context_menu(Scene& scene, SceneNode* node, Group* parent)
{
    if (ImGui::BeginPopupContextItem(("ContextMenu_" + std::to_string(node->id)).c_str()))
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

        if (ImGui::MenuItem(ICON_LC_FILE_PLUS "AddItem"))
        {
            auto item = scene.create_node<Item>(target_group);
            item->name = generate_unique_name(scene, "Cube");
            clear_selection(&scene.root);
            item->selected = true;
            rename_node_id = item->id;
        }

        if (ImGui::MenuItem(ICON_LC_FOLDER_PLUS "AddGroup"))
        {
            auto group = scene.create_node<Group>(target_group);
            group->name = generate_unique_name(scene, "Group");
            clear_selection(&scene.root);
            group->selected = true;
            rename_node_id = group->id;
        }

        if (node)
        {
            ImGui::Separator();
            if (ImGui::MenuItem(ICON_LC_PENCIL "Rename"))
            {
                rename_node_id = node->id;
            }

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
            if (ImGui::MenuItem(ICON_LC_TRASH_2 "Delete"))
            {
                Group* p = parent ? parent : &scene.root;
                auto& children = p->children;
                children.erase(
                    std::remove_if(children.begin(), children.end(),
                        [node](const std::shared_ptr<SceneNode>& child) { return child.get() == node; }),
                    children.end()
                );
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
            reorder_node(scene, dropped_node, parent, index_in_parent);
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

        if (ImGui::GetIO().KeyCtrl)
        {
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
            clear_selection(&scene.root);
            node->selected = true;
        }
    }

    if (res.right_clicked)
    {
        if (rename_node_id != -1 && rename_node_id != node->id)
            rename_node_id = -1;

        if (!node->selected)
        {
            clear_selection(&scene.root);
            node->selected = true;
        }
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

    draw_context_menu(scene, node, parent);

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
            
            reorder_node(scene, dropped_node, group_node, group_node->children.size());
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

void ObjectsPanel::draw(Scene& scene)
{
    ImGui::Begin(ICON_LC_BOXES "Objects###ObjectsPanel");

    auto root_children = scene.root.children;
    for (size_t i = 0; i < root_children.size(); ++i)
    {
        draw_node(scene, root_children[i].get(), &scene.root, i);
    }

    ImGui::Spacing();
    ImGui::Dummy(ImGui::GetContentRegionAvail());
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        clear_selection(&scene.root);

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_NODE"))
        {
            SceneNode* dropped_node = *static_cast<SceneNode**>(payload->Data);
            reorder_node(scene, dropped_node, &scene.root, scene.root.children.size());
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginPopupContextWindow("PanelContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem(ICON_LC_FILE_PLUS "AddItem"))
        {
            auto item = scene.create_node<Item>(&scene.root);
            item->name = generate_unique_name(scene, "Cube");
            clear_selection(&scene.root);
            item->selected = true;
            rename_node_id = item->id;
        }
        if (ImGui::MenuItem(ICON_LC_FOLDER_PLUS "AddGroup"))
        {
            auto group = scene.create_node<Group>(&scene.root);
            group->name = generate_unique_name(scene, "Group");
            clear_selection(&scene.root);
            group->selected = true;
            rename_node_id = group->id;
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}