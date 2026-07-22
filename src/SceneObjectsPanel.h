#pragma once

#include <Scene.h>

#include <memory>
#include <vector>

class SceneObjectsPanel
{
private:
    enum class SceneDropMode
    {
        Before,
        After,
        Inside
    };

    int rename_node_id = -1;

    void clear_selection(SceneNode* node);
    void clear_selection(Scene& scene);
    void collect_selected_nodes(SceneNode* node, std::vector<SceneNode*>& selected_nodes);
    void select_single_node(Scene& scene, SceneNode* node);
    Group* get_selected_group(Scene& scene) const;
    bool find_parent_group(Group* current, SceneNode* target, Group*& parent_group, size_t& child_index);
    bool is_descendant_of(const Group* candidate_parent, const SceneNode* node) const;
    bool can_drop_node(SceneNode* node, Group* new_parent) const;
    bool move_node(Scene& scene, SceneNode* node, Group* new_parent, size_t insert_index);
    bool reparent_node(Scene& scene, SceneNode* node, Group* new_parent);
    void handle_root_insert_target(Scene& scene, size_t insert_index);
    void handle_scene_drop_target(Scene& scene, SceneNode* target_node, Group* target_parent, size_t target_index, bool allow_inside_drop);
    void draw_scene_node_row(Scene& scene, SceneNode* node, int depth);

public:
    void draw(Scene& scene);
};