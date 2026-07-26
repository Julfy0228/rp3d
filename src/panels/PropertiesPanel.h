#pragma once

#include <Scene.h>

#include <glm/glm.hpp>

#include <vector>

class UndoManager;

class PropertiesPanel
{
private:
    void collect_selected_roots(SceneNode* node, std::vector<SceneNode*>& selected_nodes, bool ancestor_selected);
    glm::i32vec3 compute_selection_center(const std::vector<SceneNode*>& selected_roots) const;
    void capture_undo_once(Scene& scene, UndoManager* undo_manager, bool& snapshot_captured) const;

public:
    void draw(Scene& scene, UndoManager* undo_manager);
};