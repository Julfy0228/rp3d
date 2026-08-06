#pragma once

#include <Scene.h>
#include <memory>
#include <ImGuizmo.h>

class Renderer;
class UndoManager;

class ScenePanel
{
public:
    ScenePanel();
    ~ScenePanel();

    bool init();
    void shutdown();
    void draw(Scene& scene, UndoManager* undo_manager);

private:
    std::unique_ptr<Renderer> renderer;
    ImGuizmo::OPERATION gizmo_operation = ImGuizmo::TRANSLATE;
    glm::mat4 gizmo_active_matrix = glm::mat4(1.0f);
    bool gizmo_was_using = false;
    
    glm::i32vec2 gizmo_scale_base_size = glm::i32vec2(1, 1);
    std::vector<glm::i32vec2> gizmo_scale_base_vertices;
    int gizmo_scale_base_thickness = 1;
    glm::vec3 gizmo_scale_prev_ratio = glm::vec3(1.0f);
    glm::i32vec3 gizmo_base_position = glm::i32vec3(0, 0, 0);
    glm::i32vec3 gizmo_base_rotation = glm::i32vec3(0, 0, 0);
    glm::i32vec3 gizmo_prev_position = glm::i32vec3(0, 0, 0);
    glm::i32vec3 gizmo_prev_rotation = glm::i32vec3(0, 0, 0);
    ImGuizmo::MOVETYPE gizmo_active_axis_type = ImGuizmo::MT_NONE;
    int gizmo_active_axis = -1;
};