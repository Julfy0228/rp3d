#include "UndoManager.h"

void UndoManager::capture_snapshot(const Scene& scene)
{
    undo_stack.push_back(make_snapshot(scene));
    redo_stack.clear();
}

bool UndoManager::undo(Scene& scene)
{
    if (undo_stack.empty())
        return false;

    redo_stack.push_back(make_snapshot(scene));
    restore_snapshot(scene, undo_stack.back());
    undo_stack.pop_back();
    return true;
}

bool UndoManager::redo(Scene& scene)
{
    if (redo_stack.empty())
        return false;

    undo_stack.push_back(make_snapshot(scene));
    restore_snapshot(scene, redo_stack.back());
    redo_stack.pop_back();
    return true;
}

bool UndoManager::can_undo() const
{
    return !undo_stack.empty();
}

bool UndoManager::can_redo() const
{
    return !redo_stack.empty();
}

void UndoManager::clear()
{
    undo_stack.clear();
    redo_stack.clear();
}

UndoManager::Snapshot UndoManager::make_snapshot(const Scene& scene) const
{
    Snapshot snapshot;
    snapshot.next_id = scene.next_id;
    snapshot.root = make_snapshot_node(scene.root);
    return snapshot;
}

UndoManager::SnapshotNode UndoManager::make_snapshot_node(const SceneNode& node) const
{
    SnapshotNode snapshot_node;
    snapshot_node.is_group = node.is_group();
    snapshot_node.id = node.id;
    snapshot_node.name = node.name;
    snapshot_node.visible = node.visible;
    snapshot_node.selected = node.selected;
    snapshot_node.position = node.position;
    snapshot_node.rotation = node.rotation;

    if (node.is_group())
    {
        const Group& group = static_cast<const Group&>(node);
        snapshot_node.children.reserve(group.children.size());
        for (const auto& child : group.children)
        {
            if (child)
                snapshot_node.children.push_back(make_snapshot_node(*child));
        }
    }
    else
    {
        const Item& item = static_cast<const Item&>(node);
        snapshot_node.vertices = item.vertices;
        snapshot_node.thickness = item.thickness;
        snapshot_node.color = item.color;
    }

    return snapshot_node;
}

void UndoManager::restore_snapshot(Scene& scene, const Snapshot& snapshot) const
{
    scene.next_id = snapshot.next_id;
    scene.root = Group();
    scene.root.id = snapshot.root.id;
    scene.root.name = snapshot.root.name;
    scene.root.visible = snapshot.root.visible;
    scene.root.selected = snapshot.root.selected;
    scene.root.position = snapshot.root.position;
    scene.root.rotation = snapshot.root.rotation;
    scene.root.children.clear();
    scene.root.children.reserve(snapshot.root.children.size());

    for (const SnapshotNode& child : snapshot.root.children)
        scene.root.children.push_back(restore_snapshot_node(child));
}

std::shared_ptr<SceneNode> UndoManager::restore_snapshot_node(const SnapshotNode& snapshot_node) const
{
    if (snapshot_node.is_group)
    {
        auto group = std::make_shared<Group>();
        group->id = snapshot_node.id;
        group->name = snapshot_node.name;
        group->visible = snapshot_node.visible;
        group->selected = snapshot_node.selected;
        group->position = snapshot_node.position;
        group->rotation = snapshot_node.rotation;
        group->children.reserve(snapshot_node.children.size());
        for (const SnapshotNode& child : snapshot_node.children)
            group->children.push_back(restore_snapshot_node(child));
        return group;
    }

    auto item = std::make_shared<Item>();
    item->id = snapshot_node.id;
    item->name = snapshot_node.name;
    item->visible = snapshot_node.visible;
    item->selected = snapshot_node.selected;
    item->position = snapshot_node.position;
    item->rotation = snapshot_node.rotation;
    item->vertices = snapshot_node.vertices;
    item->thickness = snapshot_node.thickness;
    item->color = snapshot_node.color;
    return item;
}