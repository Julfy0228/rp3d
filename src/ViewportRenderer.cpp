#include "ViewportRenderer.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <array>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <cstdint>
#include <optional>
#include <unordered_set>
#include <vector>

#include <mapbox/earcut.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace
{
    constexpr double kEpsilon = 1e-6;
    constexpr double kTwoPi = 6.28318530717958647692;
    constexpr std::uint64_t kFnvOffsetBasis = 1469598103934665603ull;
    constexpr std::uint64_t kFnvPrime = 1099511628211ull;

    struct Point2d
    {
        double x = 0.0;
        double y = 0.0;
    };

    struct SegmentSplitPoint
    {
        double t = 0.0;
        Point2d point;
    };

    bool nearly_equal(double a, double b)
    {
        return std::abs(a - b) <= kEpsilon;
    }

    bool same_point(const Point2d& a, const Point2d& b)
    {
        return nearly_equal(a.x, b.x) && nearly_equal(a.y, b.y);
    }

    double signed_area(const std::vector<Point2d>& polygon)
    {
        double area = 0.0;
        if (polygon.size() < 3)
            return 0.0;

        for (size_t i = 0; i < polygon.size(); ++i)
        {
            const Point2d& a = polygon[i];
            const Point2d& b = polygon[(i + 1) % polygon.size()];
            area += a.x * b.y - b.x * a.y;
        }

        return area * 0.5;
    }

    bool intersect_segments(
        const Point2d& a,
        const Point2d& b,
        const Point2d& c,
        const Point2d& d,
        double& t_ab,
        double& t_cd,
        Point2d& intersection)
    {
        double abx = b.x - a.x;
        double aby = b.y - a.y;
        double cdx = d.x - c.x;
        double cdy = d.y - c.y;
        double denom = abx * cdy - aby * cdx;
        if (std::abs(denom) <= kEpsilon)
            return false;

        double acx = c.x - a.x;
        double acy = c.y - a.y;
        t_ab = (acx * cdy - acy * cdx) / denom;
        t_cd = (acx * aby - acy * abx) / denom;
        if (t_ab <= kEpsilon || t_ab >= 1.0 - kEpsilon || t_cd <= kEpsilon || t_cd >= 1.0 - kEpsilon)
            return false;

        intersection = { a.x + abx * t_ab, a.y + aby * t_ab };
        return true;
    }

    std::vector<std::vector<Point2d>> build_simple_loops(const std::vector<glm::i32vec2>& contour)
    {
        std::vector<std::vector<Point2d>> loops;
        if (contour.size() < 3)
            return loops;

        std::vector<Point2d> base_points;
        base_points.reserve(contour.size());
        for (const glm::i32vec2& point : contour)
            base_points.push_back({ static_cast<double>(point.x), static_cast<double>(point.y) });

        const size_t edge_count = base_points.size();
        std::vector<std::vector<SegmentSplitPoint>> splits(edge_count);
        for (size_t i = 0; i < edge_count; ++i)
        {
            splits[i].push_back({ 0.0, base_points[i] });
            splits[i].push_back({ 1.0, base_points[(i + 1) % edge_count] });
        }

        for (size_t i = 0; i < edge_count; ++i)
        {
            size_t i_next = (i + 1) % edge_count;
            for (size_t j = i + 1; j < edge_count; ++j)
            {
                size_t j_next = (j + 1) % edge_count;
                if (i == j || i_next == j || j_next == i)
                    continue;
                if (i == 0 && j_next == 0)
                    continue;

                double t_i = 0.0;
                double t_j = 0.0;
                Point2d intersection;
                if (intersect_segments(base_points[i], base_points[i_next], base_points[j], base_points[j_next], t_i, t_j, intersection)) {
                    splits[i].push_back({ t_i, intersection });
                    splits[j].push_back({ t_j, intersection });
                }
            }
        }

        std::vector<Point2d> graph_points;
        auto register_point = [&](const Point2d& point) -> int
        {
            for (size_t index = 0; index < graph_points.size(); ++index)
                if (same_point(graph_points[index], point))
                    return static_cast<int>(index);
            graph_points.push_back(point);
            return static_cast<int>(graph_points.size() - 1);
        };

        std::map<int, std::vector<int>> adjacency;
        for (size_t edge_index = 0; edge_index < edge_count; ++edge_index)
        {
            auto& edge_splits = splits[edge_index];
            std::sort(edge_splits.begin(), edge_splits.end(), [](const SegmentSplitPoint& lhs, const SegmentSplitPoint& rhs)
            {
                return lhs.t < rhs.t;
            });

            std::vector<int> chain;
            for (const SegmentSplitPoint& split : edge_splits)
            {
                int point_index = register_point(split.point);
                if (chain.empty() || chain.back() != point_index)
                    chain.push_back(point_index);
            }

            for (size_t i = 0; i + 1 < chain.size(); ++i)
            {
                int a = chain[i];
                int b = chain[i + 1];
                if (a == b) continue;
                adjacency[a].push_back(b);
                adjacency[b].push_back(a);
            }
        }

        std::set<std::pair<int, int>> used_directed_edges;
        for (const auto& [start, neighbors] : adjacency)
            for (int next : neighbors) {
                if (used_directed_edges.find({ start, next }) != used_directed_edges.end())
                    continue;

                std::vector<int> cycle_indices;
                cycle_indices.push_back(start);
                int previous = start;
                int current = next;
                used_directed_edges.insert({ start, next });

                while (true) {
                    cycle_indices.push_back(current);
                    const Point2d& current_point = graph_points[current];
                    const Point2d& previous_point = graph_points[previous];
                    double incoming_angle = std::atan2(previous_point.y - current_point.y, previous_point.x - current_point.x);

                    int best_neighbor = -1;
                    double best_turn = std::numeric_limits<double>::max();
                    for (int candidate : adjacency[current]) {
                        if (candidate == previous) {
                            continue;
                        }

                        const Point2d& candidate_point = graph_points[candidate];
                        double outgoing_angle = std::atan2(candidate_point.y - current_point.y, candidate_point.x - current_point.x);
                        double turn = std::fmod((outgoing_angle - incoming_angle) + kTwoPi, kTwoPi);
                        if (turn < best_turn) {
                            best_turn = turn;
                            best_neighbor = candidate;
                        }
                    }

                    if (best_neighbor == -1) {
                        cycle_indices.clear();
                        break;
                    }

                    if (best_neighbor == cycle_indices.front()) {
                        used_directed_edges.insert({ current, best_neighbor });
                        break;
                    }

                    if (used_directed_edges.find({ current, best_neighbor }) != used_directed_edges.end()) {
                        cycle_indices.clear();
                        break;
                    }

                    used_directed_edges.insert({ current, best_neighbor });
                    previous = current;
                    current = best_neighbor;
                }

                if (cycle_indices.size() < 3) {
                    continue;
                }

                std::vector<Point2d> loop;
                loop.reserve(cycle_indices.size());
                for (int index : cycle_indices)
                    if (loop.empty() || !same_point(loop.back(), graph_points[index]))
                        loop.push_back(graph_points[index]);

                if (loop.size() >= 3 && std::abs(signed_area(loop)) > kEpsilon)
                    loops.push_back(loop);
            }

        if (loops.empty())
        {
            std::vector<Point2d> fallback = base_points;
            if (std::abs(signed_area(fallback)) > kEpsilon)
                loops.push_back(fallback);
        }

        return loops;
    }

    std::string read_text_file(const char* path)
    {
        std::ifstream file(path, std::ios::in | std::ios::binary);
        if (!file) return {};

        std::ostringstream stream;
        stream << file.rdbuf();
        return stream.str();
    }

    GLuint compile_shader(GLenum shader_type, const std::string& source)
    {
        GLuint shader = glCreateShader(shader_type);
        const char* source_ptr = source.c_str();
        glShaderSource(shader, 1, &source_ptr, nullptr);
        glCompileShader(shader);

        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == GL_TRUE)
            return shader;

        GLint log_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        std::string log(static_cast<size_t>(log_length), '\0');
        glGetShaderInfoLog(shader, log_length, nullptr, log.data());
        std::cerr << "Shader compilation failed: " << log << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    void append_triangle(
        std::vector<ViewportRenderer::MeshVertex>& vertices,
        std::vector<unsigned int>& indices,
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c)
    {
        glm::vec3 local_a = a;
        glm::vec3 local_b = b;
        glm::vec3 local_c = c;
        glm::vec3 normal = glm::normalize(glm::cross(local_b - local_a, local_c - local_a));
        unsigned int base = static_cast<unsigned int>(vertices.size());
        vertices.push_back({ local_a, normal });
        vertices.push_back({ local_b, normal });
        vertices.push_back({ local_c, normal });
        indices.push_back(base);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
    }

    void hash_u32(std::uint64_t& hash, std::uint32_t value)
    {
        hash ^= static_cast<std::uint64_t>(value);
        hash *= kFnvPrime;
    }

    void collect_selected_roots_for_viewport(const Group& group, std::vector<const SceneNode*>& selected_nodes, bool ancestor_selected)
    {
        for (const auto& child : group.children)
        {
            if (!child || !child->visible)
                continue;

            const bool child_selected = child->selected;
            if (child_selected && !ancestor_selected)
                selected_nodes.push_back(child.get());

            if (child->is_group())
            {
                collect_selected_roots_for_viewport(
                    *static_cast<const Group*>(child.get()),
                    selected_nodes,
                    ancestor_selected || child_selected);
            }
        }
    }

}

bool ViewportRenderer::init()
{
    glEnable(GL_DEPTH_TEST);
    return load_shader_program("assets/shaders/scene.vert", "assets/shaders/scene.frag");
}

void ViewportRenderer::shutdown()
{
    mesh_cache.clear();
    selection_center_screen_position.reset();

    if (mesh_ebo != 0) glDeleteBuffers(1, &mesh_ebo);
    if (mesh_vbo != 0) glDeleteBuffers(1, &mesh_vbo);
    if (mesh_vao != 0) glDeleteVertexArrays(1, &mesh_vao);
    if (shader_program != 0) glDeleteProgram(shader_program);
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
    width = 0;
    height = 0;
}

std::uint64_t ViewportRenderer::compute_item_mesh_signature(const Item& item) const
{
    std::uint64_t hash = kFnvOffsetBasis;
    hash_u32(hash, static_cast<std::uint32_t>(item.thickness));
    hash_u32(hash, static_cast<std::uint32_t>(item.vertices.size()));

    for (const glm::i32vec2& point : item.vertices)
    {
        hash_u32(hash, static_cast<std::uint32_t>(point.x));
        hash_u32(hash, static_cast<std::uint32_t>(point.y));
    }

    return hash;
}

const ViewportRenderer::CachedMesh* ViewportRenderer::get_cached_item_mesh(const Item& item) const
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
    cache_entry.valid = build_item_mesh(item, cache_entry.vertices, cache_entry.indices, cache_entry.pivot);
    if (!cache_entry.valid)
        return nullptr;

    return &cache_entry;
}

bool ViewportRenderer::load_shader_program(const char* vertex_path, const char* fragment_path)
{
    std::string vertex_source = read_text_file(vertex_path);
    std::string fragment_source = read_text_file(fragment_path);
    if (vertex_source.empty() || fragment_source.empty())
    {
        std::cerr << "Failed to read shader sources" << std::endl;
        return false;
    }

    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_source);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
    if (vertex_shader == 0 || fragment_shader == 0)
    {
        if (vertex_shader != 0) glDeleteShader(vertex_shader);
        if (fragment_shader != 0) glDeleteShader(fragment_shader);
        return false;
    }

    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    GLint success = GL_FALSE;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (success == GL_TRUE)
        return true;

    GLint log_length = 0;
    glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &log_length);
    std::string log(static_cast<size_t>(log_length), '\0');
    glGetProgramInfoLog(shader_program, log_length, nullptr, log.data());
    std::cerr << "Program link failed: " << log << std::endl;
    glDeleteProgram(shader_program);
    shader_program = 0;
    return false;
}

void ViewportRenderer::ensure_viewport_resources(int target_width, int target_height)
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

void ViewportRenderer::ensure_mesh_resources()
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

glm::vec3 ViewportRenderer::compute_item_pivot(const Item& item) const
{
    if (item.vertices.empty())
        return glm::vec3(0.0f);

    int min_x = item.vertices[0].x;
    int max_x = item.vertices[0].x;
    int min_y = item.vertices[0].y;
    int max_y = item.vertices[0].y;

    for (const glm::i32vec2& point : item.vertices)
    {
        min_x = std::min(min_x, point.x);
        max_x = std::max(max_x, point.x);
        min_y = std::min(min_y, point.y);
        max_y = std::max(max_y, point.y);
    }

    return glm::vec3(
        (static_cast<float>(min_x) + static_cast<float>(max_x)) * 0.5f,
        (static_cast<float>(min_y) + static_cast<float>(max_y)) * 0.5f,
        0.0f);
}

bool ViewportRenderer::build_item_mesh(const Item& item, std::vector<MeshVertex>& vertices, std::vector<unsigned int>& indices, glm::vec3& pivot) const
{
    if (item.vertices.size() < 3)
        return false;

    const int height = std::max(0, item.thickness);
    const float half_height = static_cast<float>(height) * 0.5f;

    pivot = compute_item_pivot(item);

    std::vector<std::vector<Point2d>> loops = build_simple_loops(item.vertices);

    for (const std::vector<Point2d>& loop : loops)
    {
        if (loop.size() < 3)
            continue;

        std::vector<std::vector<std::array<double, 2>>> polygon(1);
        polygon[0].reserve(loop.size());
        for (const Point2d& point : loop)
            polygon[0].push_back({ static_cast<double>(point.x), static_cast<double>(point.y) });

        std::vector<uint32_t> cap_indices = mapbox::earcut<uint32_t>(polygon);

        for (size_t i = 0; i + 2 < cap_indices.size(); i += 3)
        {
            const Point2d& a2 = loop[cap_indices[i]];
            const Point2d& b2 = loop[cap_indices[i + 1]];
            const Point2d& c2 = loop[cap_indices[i + 2]];

            glm::vec3 a(static_cast<float>(a2.x), static_cast<float>(a2.y), height > 0 ? -half_height : 0.0f);
            glm::vec3 b(static_cast<float>(b2.x), static_cast<float>(b2.y), height > 0 ? -half_height : 0.0f);
            glm::vec3 c(static_cast<float>(c2.x), static_cast<float>(c2.y), height > 0 ? -half_height : 0.0f);

            if (height == 0)
                append_triangle(vertices, indices, a, b, c);
            else
                append_triangle(vertices, indices, c, b, a);

            if (height > 0)
            {
                glm::vec3 a_top(a.x, a.y, half_height);
                glm::vec3 b_top(b.x, b.y, half_height);
                glm::vec3 c_top(c.x, c.y, half_height);

                append_triangle(vertices, indices, a_top, b_top, c_top);
            }
        }

        if (height > 0)
        {
            const size_t loop_count = loop.size();
            for (size_t i = 0; i < loop_count; ++i)
            {
                size_t next = (i + 1) % loop_count;

                glm::vec3 bottom_a(static_cast<float>(loop[i].x), static_cast<float>(loop[i].y), -half_height);
                glm::vec3 bottom_b(static_cast<float>(loop[next].x), static_cast<float>(loop[next].y), -half_height);
                glm::vec3 top_a(bottom_a.x, bottom_a.y, half_height);
                glm::vec3 top_b(bottom_b.x, bottom_b.y, half_height);

                append_triangle(vertices, indices, bottom_a, bottom_b, top_b);
                append_triangle(vertices, indices, bottom_a, top_b, top_a);
            }
        }
    }

    for (MeshVertex& vertex : vertices)
        vertex.position -= pivot;

    return !vertices.empty();
}

glm::mat4 ViewportRenderer::build_node_transform(const SceneNode& node) const
{
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(node.position.x, node.position.y, node.position.z));
    transform = glm::rotate(transform, glm::radians(static_cast<float>(node.rotation.z)), glm::vec3(0.0f, 0.0f, 1.0f));
    transform = glm::rotate(transform, glm::radians(static_cast<float>(node.rotation.y)), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(static_cast<float>(node.rotation.x)), glm::vec3(1.0f, 0.0f, 0.0f));
    return transform;
}

glm::mat4 ViewportRenderer::build_model_matrix(const Item& item, const glm::mat4& parent_transform, const glm::vec3& pivot) const
{
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(item.position) + pivot);
    transform = glm::rotate(transform, glm::radians(static_cast<float>(item.rotation.z)), glm::vec3(0.0f, 0.0f, 1.0f));
    transform = glm::rotate(transform, glm::radians(static_cast<float>(item.rotation.y)), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(static_cast<float>(item.rotation.x)), glm::vec3(1.0f, 0.0f, 0.0f));
    return parent_transform * transform;
}

void ViewportRenderer::collect_items(const Group& group, const glm::mat4& parent_transform, std::vector<ItemRenderData>& items) const
{
    glm::mat4 group_transform = parent_transform;
    if (group.id != 0) {
        group_transform = parent_transform * build_node_transform(group);
    }

    for (const auto& child : group.children) {
        if (!child || !child->visible)
            continue;

        if (child->is_group())
            collect_items(*static_cast<const Group*>(child.get()), group_transform, items);
        else
        {
            ItemRenderData item_data;
            item_data.item = static_cast<const Item*>(child.get());
            item_data.pivot = compute_item_pivot(*item_data.item);
            item_data.model = build_model_matrix(*item_data.item, group_transform, item_data.pivot);
            item_data.world_center = glm::vec3(item_data.model[3]);
            items.push_back(item_data);
        }
    }
}

void ViewportRenderer::render(const Scene& scene, int target_width, int target_height)
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

    glm::vec3 target = camera.target;
    glm::vec3 eye(
        target.x + std::cos(camera.yaw) * std::cos(camera.pitch) * camera.distance,
        target.y + std::sin(camera.yaw) * std::cos(camera.pitch) * camera.distance,
        target.z + std::sin(camera.pitch) * camera.distance);

    glm::mat4 view = glm::lookAt(eye, target, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), static_cast<float>(target_width) / static_cast<float>(target_height), 0.1f, 1000.0f);

    std::vector<const SceneNode*> selected_nodes;
    collect_selected_roots_for_viewport(scene.root, selected_nodes, false);
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
                Group* parent_group = nullptr;

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

    glUseProgram(shader_program);
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(glGetUniformLocation(shader_program, "uLightDirection"), 0.45f, -0.35f, 0.82f);

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

ImTextureID ViewportRenderer::get_texture_id() const
{
    return static_cast<ImTextureID>(color_texture);
}

std::optional<glm::vec2> ViewportRenderer::get_selection_center_screen_position() const
{
    return selection_center_screen_position;
}