#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Scene.h>
#include <UndoManager.h>

class ViewportRenderer;
class ObjectsPanel;
class ScenePanel;
class StylePanel;
class PropertiesPanel;

class EditorApp
{
private:
    GLFWwindow* window = nullptr;
    Scene scene;
    UndoManager undo_manager;
    ViewportRenderer* viewport_renderer = nullptr;
    ObjectsPanel* objects_panel = nullptr;
    ScenePanel* scene_panel = nullptr;
    StylePanel* style_panel = nullptr;
    PropertiesPanel* properties_panel = nullptr;

    bool init_window();
    void init_imgui();

    void draw();
    void draw_dockspace();
    void draw_panels();
    
public:
    EditorApp() = default;
    ~EditorApp();

    bool init();

    void run();
    void shutdown();
};
