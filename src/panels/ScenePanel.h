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
};