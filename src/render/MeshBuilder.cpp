#include "MeshBuilder.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <set>

#include <mapbox/earcut.hpp>

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
                if (intersect_segments(base_points[i], base_points[i_next], base_points[j], base_points[j_next], t_i, t_j, intersection))
                {
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
                if (a == b)
                    continue;
                adjacency[a].push_back(b);
                adjacency[b].push_back(a);
            }
        }

        std::set<std::pair<int, int>> used_directed_edges;
        for (const auto& [start, neighbors] : adjacency)
        {
            for (int next : neighbors)
            {
                if (used_directed_edges.find({ start, next }) != used_directed_edges.end())
                    continue;

                std::vector<int> cycle_indices;
                cycle_indices.push_back(start);
                int previous = start;
                int current = next;
                used_directed_edges.insert({ start, next });

                while (true)
                {
                    cycle_indices.push_back(current);
                    const Point2d& current_point = graph_points[current];
                    const Point2d& previous_point = graph_points[previous];
                    double incoming_angle = std::atan2(previous_point.y - current_point.y, previous_point.x - current_point.x);

                    int best_neighbor = -1;
                    double best_turn = std::numeric_limits<double>::max();
                    for (int candidate : adjacency[current])
                    {
                        if (candidate == previous)
                            continue;

                        const Point2d& candidate_point = graph_points[candidate];
                        double outgoing_angle = std::atan2(candidate_point.y - current_point.y, candidate_point.x - current_point.x);
                        double turn = std::fmod((outgoing_angle - incoming_angle) + kTwoPi, kTwoPi);
                        if (turn < best_turn)
                        {
                            best_turn = turn;
                            best_neighbor = candidate;
                        }
                    }

                    if (best_neighbor == -1)
                    {
                        cycle_indices.clear();
                        break;
                    }

                    if (best_neighbor == cycle_indices.front())
                    {
                        used_directed_edges.insert({ current, best_neighbor });
                        break;
                    }

                    if (used_directed_edges.find({ current, best_neighbor }) != used_directed_edges.end())
                    {
                        cycle_indices.clear();
                        break;
                    }

                    used_directed_edges.insert({ current, best_neighbor });
                    previous = current;
                    current = best_neighbor;
                }

                if (cycle_indices.size() < 3)
                    continue;

                std::vector<Point2d> loop;
                loop.reserve(cycle_indices.size());
                for (int index : cycle_indices)
                {
                    if (loop.empty() || !same_point(loop.back(), graph_points[index]))
                        loop.push_back(graph_points[index]);
                }

                if (loop.size() >= 3 && std::abs(signed_area(loop)) > kEpsilon)
                    loops.push_back(loop);
            }
        }

        if (loops.empty())
        {
            std::vector<Point2d> fallback = base_points;
            if (std::abs(signed_area(fallback)) > kEpsilon)
                loops.push_back(fallback);
        }

        return loops;
    }

    void append_triangle(
        std::vector<Renderer::MeshVertex>& vertices,
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
}

std::uint64_t render::compute_item_mesh_signature(const Item& item)
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

glm::vec3 render::compute_item_pivot(const Item& item)
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

bool render::build_item_mesh(
    const Item& item,
    std::vector<Renderer::MeshVertex>& vertices,
    std::vector<unsigned int>& indices,
    glm::vec3& pivot)
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

    for (Renderer::MeshVertex& vertex : vertices)
        vertex.position -= pivot;

    return !vertices.empty();
}