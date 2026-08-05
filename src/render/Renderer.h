#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <optional>
#include <imgui.h>

#include <cstdint>
#include <unordered_map>
#include <vector>

#include <Scene.h>

class Renderer
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
        glm::vec3 pivot = glm::vec3(0.0f);
        glm::vec3 world_center = glm::vec3(0.0f);
    };

    glm::vec3 ambient_color    = glm::vec3(0.10f, 0.12f, 0.16f);
    glm::vec3 light_direction  = glm::vec3(0.45f, -0.35f, 0.82f);
    glm::vec3 light_color      = glm::vec3(1.00f, 0.96f, 0.90f);

    bool  grid_visible   = true;
    float grid_cell_size = 16.0f;

private:
    struct CachedMesh
    {
        std::uint64_t signature = 0;
        std::vector<MeshVertex> vertices;
        std::vector<unsigned int> indices;
        glm::vec3 pivot = glm::vec3(0.0f);
        bool valid = false;
    };

    GLuint framebuffer        = 0;
    GLuint color_texture      = 0;
    GLuint depth_renderbuffer = 0;
    int width  = 0;
    int height = 0;
    int viewport_width  = 0;
    int viewport_height = 0;

    GLuint framebuffer_ms = 0;
    GLuint renderbuffer_color_ms = 0;
    GLuint renderbuffer_depth_ms = 0;
    GLuint framebuffer_resolve = 0;
    GLuint texture_resolve = 0;
    static constexpr int MSAA_SAMPLES = 4;

    GLuint shader_program = 0;
    GLuint mesh_vao = 0;
    GLuint mesh_vbo = 0;
    GLuint mesh_ebo = 0;

    GLuint selection_mask_shader = 0;
    GLuint outline_composite_shader = 0;
    GLuint selection_mask_framebuffer = 0;
    GLuint selection_mask_texture = 0;
    GLuint fullscreen_vao = 0;
    GLuint fullscreen_vbo = 0;

    GLuint grid_shader_program = 0;
    GLuint grid_vao = 0;

    Camera camera;
    mutable std::unordered_map<int, CachedMesh> mesh_cache;
    std::optional<glm::vec2> selection_center_screen_position;

    std::uint64_t compute_item_mesh_signature(const Item& item) const;
    const CachedMesh* get_cached_item_mesh(const Item& item) const;
    glm::vec3 compute_item_pivot(const Item& item) const;
    void ensure_viewport_resources(int width, int height);
    void ensure_mesh_resources();
    void ensure_grid_resources();
    bool build_item_mesh(const Item& item, std::vector<MeshVertex>& vertices, std::vector<unsigned int>& indices, glm::vec3& pivot) const;
    glm::mat4 build_node_transform(const SceneNode& node) const;
    glm::mat4 build_model_matrix(const Item& item, const glm::mat4& parent_transform, const glm::vec3& pivot) const;
    void collect_items(const Group& group, const glm::mat4& parent_transform, std::vector<ItemRenderData>& items) const;
    void render_grid(const glm::mat4& view, const glm::mat4& projection);
    void render_selection_mask(const Scene& scene, const glm::mat4& view, const glm::mat4& projection);
    void render_outline_composite();
    void ensure_fullscreen_resources();

public:
    bool init();
    void shutdown();
    void render(const Scene& scene, int width, int height);
    ImTextureID get_texture_id() const;
    std::optional<glm::vec2> get_selection_center_screen_position() const;
    
    Camera& get_camera() { return camera; }
    const Camera& get_camera() const { return camera; }
};