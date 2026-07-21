#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

struct Group
{
    std::string name;

    bool selected = false;

    std::vector<int> items;

    glm::i32vec3 position;
    glm::i32vec3 rotation;
};

struct Item
{
    int id = 0;
    std::string name;

    bool visible = true;
    bool selected = false;

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

    std::vector<glm::i32vec2> vertices;

    glm::i32vec3 position = glm::i32vec3(0);
    glm::i32vec3 rotation = glm::i32vec3(0);
    glm::i32vec3 size;

    glm::u8vec4 color = glm::u8vec4(255);
};

struct Scene
{
    std::vector<Group> groups;
    std::vector<Item> items;
};