#include "EditorApp.h"
#include "Widgets.h"


#include <iostream>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <ImGuizmo.h>

void EditorApp::glfw_error_callback(int error, const char* description)
{
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

bool EditorApp::init()
{
    if (!init_window()) return false;
    
    init_imgui();

    return true;
}

bool EditorApp::init_window()
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return false;
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(1280, 720, "Voxel Modeler", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    return true;
}

void EditorApp::init_imgui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImFontConfig font_config;
    font_config.OversampleH = 2;
    font_config.OversampleV = 2;
    io.Fonts->AddFontFromFileTTF(
        "assets/fonts/JetBrainsMono-Regular.ttf",
        20.0f,
        &font_config,
        io.Fonts->GetGlyphRangesCyrillic());

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void EditorApp::run()
{
    // Тест
    Item item;
    item.selected = true;
    scene.items.push_back(item);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        draw();

        ImGui::Render();
        int w = 0, h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
}

void EditorApp::draw()
{
    draw_dockspace();
    draw_properties();
}

void EditorApp::draw_dockspace()
{
    static ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoBackground;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar();

    ImGuiID dockspace_id = ImGui::GetID("MyEditorDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Close", "Ctrl+W"))
            {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
}

void EditorApp::draw_properties()
{
    ImGui::Begin("Properties");

    std::vector<Item*> selected_items;
    for (auto& item : scene.items) {
        if (item.selected) {
            selected_items.push_back(&item);
        }
    }
    
    std::vector<Group*> selected_groups;
    for (auto& group : scene.groups) {
        if (group.selected) {
            selected_groups.push_back(&group);
        }
    }

    auto ic = selected_items.size();
    auto gc = selected_groups.size();

    if (ic == 1 && gc == 0)
    {
        Item* item = selected_items.front();
        char name[128] = {};

        ImGui::InputText("Name", name, sizeof(name));

        ImGui::DragInt3("Position", &item->position.x, 0.1f);

        if (ImGui::DragInt3("Rotation", &item->rotation.x))
            for (int i = 0; i < 3; ++i)
                item->rotation[i] = (item->rotation[i] % 360 + 360) % 360;
                
        if (ImGui::DragInt3("Size", &item->size.x, 0.1f))
            for (int i = 0; i < 3; ++i)
                if (item->size[i] < 0) item->size[i] = 0;

        glm::vec4 color_float = glm::vec4(item->color) / 255.0f;
        if (ImGui::ColorEdit4("Color", &color_float.x, ImGuiColorEditFlags_Uint8))
            item->color = glm::u8vec4(color_float * 255.0f + 0.5f);

        Widgets::ContourEdit("Contour", item->vertices, item->size.x, item->size.y);
    }
    else if (ic == 0 && gc == 1)
    {
        Group* group = selected_groups.front();
    }
    else if (ic > 0 && gc > 0)
    {

    }
    
    ImGui::End();
}

EditorApp::~EditorApp()
{
    shutdown();
}

void EditorApp::shutdown()
{
    
}
