#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <Scene.h>

class EditorApp
{
private:
    GLFWwindow* window = nullptr;
    Scene scene;

    static void glfw_error_callback(int error, const char* description);
    bool init_window();
    void init_imgui();

    void draw();
    void draw_dockspace();
    void draw_properties();
public:
    EditorApp() = default;
    ~EditorApp();

    bool init();

    void run();
    void shutdown();
};
