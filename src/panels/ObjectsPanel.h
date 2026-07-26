#pragma once

#include <string>
#include <memory>
#include "Scene.h"

class ObjectsPanel
{
public:
    ObjectsPanel() = default;
    ~ObjectsPanel() = default;

    void draw(Scene& scene);

private:
    int rename_node_id = -1;

    void draw_node(Scene& scene, SceneNode* node, Group* parent, size_t index_in_parent);
    void draw_context_menu(Scene& scene, SceneNode* node, Group* parent);

    std::string generate_unique_name(const Scene& scene, const std::string& prefix) const;
    void clear_selection(SceneNode* node);
    void clear_descendant_selection(SceneNode* node);

    Group* find_parent(Group& root, const SceneNode* node) const;
    bool selection_has_same_parent(const Group& root, const SceneNode* node) const;
    bool has_selected_ancestor(const Group& group, const SceneNode* node) const;
    bool is_descendant_of(const Group* candidate, const SceneNode* node) const;
    bool reorder_node(Scene& scene, SceneNode* node, Group* target_parent, size_t target_index);
};