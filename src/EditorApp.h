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

    bool ContourEdit(const char* label, std::vector<glm::i32vec2>& vertices, int size_x, int size_y);
};
