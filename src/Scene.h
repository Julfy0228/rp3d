#pragma once

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

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
    int thickness = 8;
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
    float fov = 70.0f;
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);

    glm::vec3 get_eye_position() const
    {
        const glm::vec3& tgt = target;
        return glm::vec3(
            tgt.x + std::cos(yaw) * std::cos(pitch) * distance,
            tgt.y + std::sin(yaw) * std::cos(pitch) * distance,
            tgt.z + std::sin(pitch) * distance
        );
    }

    glm::vec3 get_forward() const
    {
        return glm::normalize(target - get_eye_position());
    }

    glm::mat4 get_view_matrix() const
    {
        return glm::lookAt(get_eye_position(), target, glm::vec3(0.0f, 0.0f, 1.0f));
    }

    glm::mat4 get_projection_matrix(float aspect_ratio) const
    {
        return glm::perspective(glm::radians(fov), aspect_ratio, 0.1f, 1000.0f);
    }
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