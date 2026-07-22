#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <imgui.h>

#include <Scene.h>

class ViewportRenderer
{
public:
    struct MeshVertex
    {
        glm::vec3 position;
        glm::vec3 normal;
    };

    struct ItemRenderData
    {
        const Item* item = nullptr;
        glm::mat4 model = glm::mat4(1.0f);
        glm::vec4 color = glm::vec4(1.0f);
        glm::vec3 world_center = glm::vec3(0.0f);
    };

private:

    GLuint framebuffer = 0;
    GLuint color_texture = 0;
    GLuint depth_renderbuffer = 0;
    GLuint shader_program = 0;
    GLuint mesh_vao = 0;
    GLuint mesh_vbo = 0;
    GLuint mesh_ebo = 0;
    int width = 0;
    int height = 0;
    Camera camera;

    bool load_shader_program(const char* vertex_path, const char* fragment_path);
    void ensure_viewport_resources(int width, int height);
    void ensure_mesh_resources();
    bool build_item_mesh(const Item& item, std::vector<MeshVertex>& vertices, std::vector<unsigned int>& indices, glm::vec3& pivot) const;
    glm::mat4 build_node_transform(const SceneNode& node) const;
    glm::mat4 build_model_matrix(const Item& item, const glm::mat4& parent_transform, const glm::vec3& pivot) const;
    void collect_items(const Group& group, const glm::mat4& parent_transform, std::vector<ItemRenderData>& items) const;

public:
    bool init();
    void shutdown();
    void render(const Scene& scene, int width, int height);
    ImTextureID get_texture_id() const;
};