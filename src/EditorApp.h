#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Scene.h>

class ViewportRenderer;
class SceneObjectsPanel;

class EditorApp
{
private:
    GLFWwindow* window = nullptr;
    Scene scene;
    ViewportRenderer* viewport_renderer = nullptr;
    SceneObjectsPanel* scene_objects_panel = nullptr;

    static void glfw_error_callback(int error, const char* description);
    bool init_window();
    void init_imgui();

    void draw();
    void draw_dockspace();
    void draw_objects();
    void draw_properties();
    void draw_scene();
public:
    EditorApp() = default;
    ~EditorApp();

    bool init();

    void run();
    void shutdown();
};
