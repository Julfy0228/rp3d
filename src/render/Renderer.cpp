#include "render/Renderer.h"
#include <Colors.h>
#include "render/MeshBuilder.h"
#include "render/SceneRenderData.h"
#include "render/Shaders.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <optional>
#include <unordered_set>
#include <vector>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace
{
}

bool Renderer::init()
{
    glEnable(GL_DEPTH_TEST);
    bool ok = render::load_shader_program("assets/shaders/scene.vert", "assets/shaders/scene.frag", shader_program);
    ok &= render::load_shader_program("assets/shaders/grid.vert",  "assets/shaders/grid.frag",  grid_shader_program);
    return ok;
}

void Renderer::shutdown()
{
    mesh_cache.clear();
    selection_center_screen_position.reset();

    if (mesh_ebo != 0) glDeleteBuffers(1, &mesh_ebo);
    if (mesh_vbo != 0) glDeleteBuffers(1, &mesh_vbo);
    if (mesh_vao != 0) glDeleteVertexArrays(1, &mesh_vao);
    if (shader_program != 0) glDeleteProgram(shader_program);

    if (grid_vao != 0) glDeleteVertexArrays(1, &grid_vao);
    if (grid_shader_program != 0) glDeleteProgram(grid_shader_program);

    if (depth_renderbuffer != 0) glDeleteRenderbuffers(1, &depth_renderbuffer);
    if (color_texture != 0) glDeleteTextures(1, &color_texture);
    if (framebuffer != 0) glDeleteFramebuffers(1, &framebuffer);

    framebuffer = 0;
    color_texture = 0;
    depth_renderbuffer = 0;
    shader_program = 0;
    mesh_vao = 0;
    mesh_vbo = 0;
    mesh_ebo = 0;
    grid_shader_program = 0;
    grid_vao = 0;
    width = 0;
    height = 0;
}

std::uint64_t Renderer::compute_item_mesh_signature(const Item& item) const
{
    return render::compute_item_mesh_signature(item);
}

const Renderer::CachedMesh* Renderer::get_cached_item_mesh(const Item& item) const
{
    const std::uint64_t signature = compute_item_mesh_signature(item);
    auto cache_it = mesh_cache.find(item.id);
    if (cache_it != mesh_cache.end() && cache_it->second.valid && cache_it->second.signature == signature)
        return &cache_it->second;

    auto [inserted_it, inserted] = mesh_cache.try_emplace(item.id);
    CachedMesh& cache_entry = inserted_it->second;

    cache_entry.vertices.clear();
    cache_entry.indices.clear();
    cache_entry.pivot = glm::vec3(0.0f);
    cache_entry.signature = signature;
    cache_entry.valid = render::build_item_mesh(item, cache_entry.vertices, cache_entry.indices, cache_entry.pivot);
    if (!cache_entry.valid)
        return nullptr;

    return &cache_entry;
}

void Renderer::ensure_viewport_resources(int target_width, int target_height)
{
    if (target_width <= 0 || target_height <= 0)
        return;

    if (width == target_width && height == target_height && framebuffer != 0)
        return;

    if (framebuffer != 0)
    {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &color_texture);
        glDeleteRenderbuffers(1, &depth_renderbuffer);
        framebuffer = 0;
        color_texture = 0;
        depth_renderbuffer = 0;
    }

    width = target_width;
    height = target_height;

    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glGenTextures(1, &color_texture);
    glBindTexture(GL_TEXTURE_2D, color_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture, 0);

    glGenRenderbuffers(1, &depth_renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_renderbuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Viewport framebuffer is incomplete" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::ensure_mesh_resources()
{
    if (mesh_vao != 0)
        return;

    glGenVertexArrays(1, &mesh_vao);
    glGenBuffers(1, &mesh_vbo);
    glGenBuffers(1, &mesh_ebo);

    glBindVertexArray(mesh_vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_ebo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), reinterpret_cast<void*>(offsetof(MeshVertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), reinterpret_cast<void*>(offsetof(MeshVertex, normal)));
    glBindVertexArray(0);
}

glm::vec3 Renderer::compute_item_pivot(const Item& item) const
{
    return render::compute_item_pivot(item);
}

bool Renderer::build_item_mesh(const Item& item, std::vector<MeshVertex>& vertices, std::vector<unsigned int>& indices, glm::vec3& pivot) const
{
    return render::build_item_mesh(item, vertices, indices, pivot);
}

glm::mat4 Renderer::build_node_transform(const SceneNode& node) const
{
    return render::build_node_transform(node);
}

glm::mat4 Renderer::build_model_matrix(const Item& item, const glm::mat4& parent_transform, const glm::vec3& pivot) const
{
    return render::build_model_matrix(item, parent_transform, pivot);
}

void Renderer::collect_items(const Group& group, const glm::mat4& parent_transform, std::vector<ItemRenderData>& items) const
{
    render::collect_items(group, parent_transform, items);
}

void Renderer::ensure_grid_resources()
{
    if (grid_vao != 0)
        return;

    glGenVertexArrays(1, &grid_vao);
}

void Renderer::render_grid(const glm::mat4& view, const glm::mat4& projection)
{
    if (!grid_visible || grid_shader_program == 0)
        return;

    ensure_grid_resources();

    glUseProgram(grid_shader_program);
    glUniformMatrix4fv(glGetUniformLocation(grid_shader_program, "uView"),       1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(grid_shader_program, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1f(glGetUniformLocation(grid_shader_program, "uCellSize"),      grid_cell_size);
    glUniform1f(glGetUniformLocation(grid_shader_program, "uFadeDistance"),  grid_cell_size * 80.0f);
    glUniform4fv(glGetUniformLocation(grid_shader_program, "uGridColorMinor"), 1, glm::value_ptr(Colors::ViewportGridMinor));
    glUniform4fv(glGetUniformLocation(grid_shader_program, "uGridColorMajor"), 1, glm::value_ptr(Colors::ViewportGridMajor));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    glBindVertexArray(grid_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

void Renderer::render(const Scene& scene, int target_width, int target_height)
{
    selection_center_screen_position.reset();

    ensure_viewport_resources(target_width, target_height);
    if (framebuffer == 0 || shader_program == 0)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, target_width, target_height);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    std::vector<ItemRenderData> items;
    collect_items(scene.root, glm::mat4(1.0f), items);
    if (items.empty())
    {
        mesh_cache.clear();

        glm::vec3 eye_empty = camera.get_eye_position();
        glm::mat4 view_empty = camera.get_view_matrix();
        glm::mat4 proj_empty = camera.get_projection_matrix(static_cast<float>(target_width) / static_cast<float>(target_height));
        render_grid(view_empty, proj_empty);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    std::unordered_set<int> live_item_ids;
    live_item_ids.reserve(items.size());
    for (const ItemRenderData& item_data : items)
        live_item_ids.insert(item_data.item->id);

    for (auto cache_it = mesh_cache.begin(); cache_it != mesh_cache.end();)
    {
        if (live_item_ids.find(cache_it->first) == live_item_ids.end())
            cache_it = mesh_cache.erase(cache_it);
        else
            ++cache_it;
    }

    ensure_mesh_resources();

    glm::mat4 view = camera.get_view_matrix();
    glm::mat4 projection = camera.get_projection_matrix(static_cast<float>(target_width) / static_cast<float>(target_height));

    std::vector<const SceneNode*> selected_nodes;
    render::collect_selected_roots_for_viewport(scene.root, selected_nodes, false);
    if (!selected_nodes.empty())
    {
        glm::vec3 world_sum(0.0f);
        int world_count = 0;
        for (const SceneNode* node : selected_nodes)
        {
            if (!node->is_group())
            {
                auto item_it = std::find_if(items.begin(), items.end(), [node](const ItemRenderData& item_data)
                {
                    return item_data.item == node;
                });
                if (item_it != items.end())
                {
                    world_sum += item_it->world_center;
                    ++world_count;
                }
            }
            else
            {
                const Group* selected_group = static_cast<const Group*>(node);
                glm::mat4 group_model = build_node_transform(*selected_group);

                auto find_parent_transform = [&](auto& self, const Group& group, const glm::mat4& parent_transform) -> bool
                {
                    glm::mat4 group_transform = parent_transform;
                    if (group.id != 0)
                        group_transform = parent_transform * build_node_transform(group);

                    for (const auto& child : group.children)
                    {
                        if (!child)
                            continue;

                        if (child.get() == selected_group)
                        {
                            group_model = group_transform * build_node_transform(*selected_group);
                            return true;
                        }

                        if (child->is_group() && self(self, *static_cast<const Group*>(child.get()), group_transform))
                            return true;
                    }

                    return false;
                };

                find_parent_transform(find_parent_transform, scene.root, glm::mat4(1.0f));
                world_sum += glm::vec3(group_model[3]);
                ++world_count;
            }
        }

        if (world_count > 0)
        {
            const glm::vec3 selection_world_center = world_sum / static_cast<float>(world_count);
            const glm::vec4 clip = projection * view * glm::vec4(selection_world_center, 1.0f);
            if (clip.w > 0.0f)
            {
                const glm::vec3 ndc = glm::vec3(clip) / clip.w;
                if (ndc.z >= -1.0f && ndc.z <= 1.0f)
                {
                    selection_center_screen_position = glm::vec2(
                        (ndc.x * 0.5f + 0.5f) * static_cast<float>(target_width),
                        (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(target_height));
                }
            }
        }
    }

    render_grid(view, projection);

    glUseProgram(shader_program);
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "uView"),       1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));

    const glm::vec3 light_dir_norm = glm::normalize(light_direction);
    glUniform3fv(glGetUniformLocation(shader_program, "uLightDirection"), 1, glm::value_ptr(light_dir_norm));
    glUniform3fv(glGetUniformLocation(shader_program, "uLightColor"),     1, glm::value_ptr(light_color));
    glUniform3fv(glGetUniformLocation(shader_program, "uAmbientColor"),   1, glm::value_ptr(ambient_color));
    glUniform3fv(glGetUniformLocation(shader_program, "uCameraPos"),      1, glm::value_ptr(camera.get_eye_position()));

    GLint model_location = glGetUniformLocation(shader_program, "uModel");
    GLint color_location = glGetUniformLocation(shader_program, "uColor");

    for (const ItemRenderData& item_data : items)
    {
        const CachedMesh* cached_mesh = get_cached_item_mesh(*item_data.item);
        if (cached_mesh == nullptr)
            continue;

        glBindVertexArray(mesh_vao);
        glBindBuffer(GL_ARRAY_BUFFER, mesh_vbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(cached_mesh->vertices.size() * sizeof(MeshVertex)), cached_mesh->vertices.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(cached_mesh->indices.size() * sizeof(unsigned int)), cached_mesh->indices.data(), GL_DYNAMIC_DRAW);

        glm::vec4 color = glm::vec4(item_data.item->color) / 255.0f;
        glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(item_data.model));
        glUniform4fv(color_location, 1, glm::value_ptr(color));
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(cached_mesh->indices.size()), GL_UNSIGNED_INT, nullptr);
    }

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ImTextureID Renderer::get_texture_id() const
{
    return static_cast<ImTextureID>(color_texture);
}

std::optional<glm::vec2> Renderer::get_selection_center_screen_position() const
{
    return selection_center_screen_position;
}