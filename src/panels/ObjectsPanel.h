#pragma once

#include <string>
#include <memory>
#include "Scene.h"

class UndoManager;

class ObjectsPanel
{
public:
    ObjectsPanel() = default;
    ~ObjectsPanel() = default;

    void draw(Scene& scene, UndoManager* undo_manager);

private:
    struct ClipboardNode
    {
        bool is_group = false;
        std::string name;
        bool visible = true;
        glm::i32vec3 position = glm::i32vec3(0);
        glm::i32vec3 rotation = glm::i32vec3(0);
        std::vector<glm::i32vec2> vertices;
        int thickness = 8;
        glm::u8vec4 color = glm::u8vec4(255);
        std::vector<ClipboardNode> children;
    };

    int rename_node_id = -1;
    std::vector<ClipboardNode> clipboard_nodes;
    bool clipboard_is_cut = false;
    int pending_single_select_node_id = -1;
    bool drag_reorder_committed = false;
    UndoManager* undo_manager = nullptr;

    void draw_node(Scene& scene, SceneNode* node, Group* parent, size_t index_in_parent);
    void draw_context_menu(Scene& scene, SceneNode* node, Group* parent, const char* popup_id);

    std::string generate_unique_name(const Scene& scene, const std::string& prefix) const;
    std::string generate_unique_copy_name(const Scene& scene, const std::string& base_name) const;
    void clear_selection(SceneNode* node);
    void clear_descendant_selection(SceneNode* node);
    void collect_selected_roots(Group& group, std::vector<SceneNode*>& roots, bool ancestor_selected = false) const;
    void collect_selected_root_parents(Group& root, const std::vector<SceneNode*>& selected_roots, std::vector<Group*>& parents) const;
    ClipboardNode make_clipboard_node(const SceneNode& node) const;
    std::shared_ptr<SceneNode> clone_from_clipboard(Scene& scene, Group& parent, const ClipboardNode& clipboard_node);
    bool remove_node(Group& root, SceneNode* node);
    void delete_selected(Scene& scene);
    void copy_selected(Scene& scene);
    void cut_selected(Scene& scene);
    void paste_selected(Scene& scene);
    void handle_shortcuts(Scene& scene);

    Group* find_parent(Group& root, const SceneNode* node) const;
    bool selection_has_same_parent(const Group& root, const SceneNode* node) const;
    bool has_selected_ancestor(const Group& group, const SceneNode* node) const;
    bool is_descendant_of(const Group* candidate, const SceneNode* node) const;
    bool reorder_node(Scene& scene, SceneNode* node, Group* target_parent, size_t target_index);
    bool reorder_selected_nodes(Scene& scene, SceneNode* dragged_node, Group* target_parent, size_t target_index);
};