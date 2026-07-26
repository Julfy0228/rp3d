#pragma once

#include "Scene.h"

#include <memory>
#include <vector>

class UndoManager
{
public:
    void capture_snapshot(const Scene& scene);
    bool undo(Scene& scene);
    bool redo(Scene& scene);
    bool can_undo() const;
    bool can_redo() const;
    void clear();

private:
    struct SnapshotNode
    {
        bool is_group = false;
        int id = 0;
        std::string name;
        bool visible = true;
        bool selected = false;
        glm::i32vec3 position = glm::i32vec3(0);
        glm::i32vec3 rotation = glm::i32vec3(0);
        std::vector<glm::i32vec2> vertices;
        int thickness = 8;
        glm::u8vec4 color = glm::u8vec4(255);
        std::vector<SnapshotNode> children;
    };

    struct Snapshot
    {
        int next_id = 1;
        SnapshotNode root;
    };

    Snapshot make_snapshot(const Scene& scene) const;
    SnapshotNode make_snapshot_node(const SceneNode& node) const;
    void restore_snapshot(Scene& scene, const Snapshot& snapshot) const;
    std::shared_ptr<SceneNode> restore_snapshot_node(const SnapshotNode& snapshot_node) const;

    std::vector<Snapshot> undo_stack;
    std::vector<Snapshot> redo_stack;
};
