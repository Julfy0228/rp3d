#pragma once

#include <string>
#include <vector>
#include <memory>
#include <algorithm>

#include <glm/glm.hpp>

struct SceneNode
{
    int id = 0;
    std::string name;

    bool visible = true;
    bool selected = false;

    glm::i32vec3 position = glm::i32vec3(0);
    glm::i32vec3 rotation = glm::i32vec3(0);

    virtual ~SceneNode() = default;

    virtual bool is_group() const { return false; }
};

struct Item : public SceneNode
{
    std::vector<glm::i32vec2> vertices;
    glm::i32vec3 size = glm::i32vec3(0);
    glm::u8vec4 color = glm::u8vec4(255);

    Item()
    {
        int side = 8;
        vertices = {
            { 0, 0 },
            { side, 0 },
            { side, side },
            { 0, side }
        };
        size = glm::i32vec3(side);
    }
};

struct Group : public SceneNode
{
    std::vector<std::shared_ptr<SceneNode>> children;

    bool is_group() const override { return true; }

    void add_child(std::shared_ptr<SceneNode> child)
    {
        if (child) {
            children.push_back(child);
        }
    }

    void remove_child(int child_id)
    {
        children.erase(
            std::remove_if(children.begin(), children.end(),
                [child_id](const std::shared_ptr<SceneNode>& node) {
                    return node->id == child_id;
                }),
            children.end()
        );
    }
};

struct Camera
{
    float yaw = 0.7f;
    float pitch = 0.45f;
    float distance = 96.0f;
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
};

struct Scene
{
    Group root;

    int next_id = 1;

    template<typename T, typename... Args>
    std::shared_ptr<T> create_node(Group* parent = nullptr, Args&&... args)
    {
        auto node = std::make_shared<T>(std::forward<Args>(args)...);
        node->id = next_id++;
        
        if (parent) {
            parent->add_child(node);
        } else {
            root.add_child(node);
        }
        
        return node;
    }
};