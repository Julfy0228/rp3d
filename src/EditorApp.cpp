#include "EditorApp.h"


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

        ContourEdit("Contour", item->vertices, item->size.x, item->size.y);
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

bool EditorApp::ContourEdit(const char* label, std::vector<glm::i32vec2>& vertices, int size_x, int size_y) {
    bool value_changed = false;

    ImGui::PushID(label);
    ImGui::Text("%s", label);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60.0f);
    if (ImGui::Button("Clear", ImVec2(60, 0))) {
        vertices.clear();
        value_changed = true;
    }

    ImVec2 canvas_p = ImGui::GetCursorScreenPos();
    ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
    if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
    
    float aspect = (float)size_y / (float)size_x;
    canvas_sz.y = canvas_sz.x * aspect;

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    draw_list->AddRectFilled(canvas_p, ImVec2(canvas_p.x + canvas_sz.x, canvas_p.y + canvas_sz.y), IM_COL32(30, 30, 35, 255));
    draw_list->AddRect(canvas_p, ImVec2(canvas_p.x + canvas_sz.x, canvas_p.y + canvas_sz.y), IM_COL32(70, 70, 80, 255));

    ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    const bool is_hovered = ImGui::IsItemHovered();
    const bool is_active = ImGui::IsItemActive();

    auto GridToScreen = [&](const glm::i32vec2& pt) -> ImVec2 {
        float norm_x = (float)pt.x / (float)size_x;
        float norm_y = (float)pt.y / (float)size_y;
        return ImVec2(canvas_p.x + norm_x * canvas_sz.x, canvas_p.y + norm_y * canvas_sz.y);
    };

    auto ScreenToGrid = [&](const ImVec2& pos) -> glm::i32vec2 {
        float norm_x = (pos.x - canvas_p.x) / canvas_sz.x;
        float norm_y = (pos.y - canvas_p.y) / canvas_sz.y;
        int gx = (int)std::round(norm_x * size_x);
        int gy = (int)std::round(norm_y * size_y);
        return glm::i32vec2(glm::clamp(gx, 0, size_x), glm::clamp(gy, 0, size_y));
    };

    const int grid_step_x = std::max(1, size_x / 16);
    const int grid_step_y = std::max(1, size_y / 16);

    for (int x = 0; x <= size_x; x += grid_step_x) {
        ImVec2 p1 = GridToScreen({x, 0});
        ImVec2 p2 = GridToScreen({x, size_y});
        draw_list->AddLine(p1, p2, IM_COL32(50, 50, 60, 150));
    }
    for (int y = 0; y <= size_y; y += grid_step_y) {
        ImVec2 p1 = GridToScreen({0, y});
        ImVec2 p2 = GridToScreen({size_x, y});
        draw_list->AddLine(p1, p2, IM_COL32(50, 50, 60, 150));
    }

    static int dragged_point_idx = -1;
    const float node_radius = 6.0f;

    if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        dragged_point_idx = -1;
        for (int i = 0; i < (int)vertices.size(); ++i)
        {
            ImVec2 node_pos = GridToScreen(vertices[i]);
            float dist_sq = (io.MousePos.x - node_pos.x) * (io.MousePos.x - node_pos.x) +
                            (io.MousePos.y - node_pos.y) * (io.MousePos.y - node_pos.y);
            if (dist_sq <= (node_radius + 4.0f) * (node_radius + 4.0f))
            {
                dragged_point_idx = i;
                break;
            }
        }

        if (dragged_point_idx == -1 && io.KeyCtrl && vertices.size() >= 2)
        {
            for (size_t i = 0; i < vertices.size(); ++i) {
                size_t next_i = (i + 1) % vertices.size();
                ImVec2 a = GridToScreen(vertices[i]);
                ImVec2 b = GridToScreen(vertices[next_i]);

                ImVec2 ap = ImVec2(io.MousePos.x - a.x, io.MousePos.y - a.y);
                ImVec2 ab = ImVec2(b.x - a.x, b.y - a.y);
                float ab2 = ab.x * ab.x + ab.y * ab.y;
                float t = (ab2 > 0.0f) ? (ap.x * ab.x + ap.y * ab.y) / ab2 : -1.0f;

                if (t >= 0.0f && t <= 1.0f)
                {
                    ImVec2 proj = ImVec2(a.x + t * ab.x, a.y + t * ab.y);
                    float dist_sq = (io.MousePos.x - proj.x) * (io.MousePos.x - proj.x) +
                                    (io.MousePos.y - proj.y) * (io.MousePos.y - proj.y);
                    if (dist_sq <= 8.0f * 8.0f)
                    {
                        glm::i32vec2 new_pt = ScreenToGrid(io.MousePos);
                        vertices.insert(vertices.begin() + i + 1, new_pt);
                        dragged_point_idx = (int)i + 1;
                        value_changed = true;
                        break;
                    }
                }
            }
        }

        if (dragged_point_idx == -1 && !io.KeyCtrl) {
            glm::i32vec2 new_pt = ScreenToGrid(io.MousePos);
            vertices.push_back(new_pt);
            dragged_point_idx = (int)vertices.size() - 1;
            value_changed = true;
        }
    }

    if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        for (int i = 0; i < (int)vertices.size(); ++i) {
            ImVec2 node_pos = GridToScreen(vertices[i]);
            float dist_sq = (io.MousePos.x - node_pos.x) * (io.MousePos.x - node_pos.x) +
                            (io.MousePos.y - node_pos.y) * (io.MousePos.y - node_pos.y);
            if (dist_sq <= (node_radius + 4.0f) * (node_radius + 4.0f)) {
                vertices.erase(vertices.begin() + i);
                value_changed = true;
                dragged_point_idx = -1;
                break;
            }
        }
    }

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && dragged_point_idx >= 0 && dragged_point_idx < (int)vertices.size()) {
        glm::i32vec2 new_grid_pos = ScreenToGrid(io.MousePos);
        if (vertices[dragged_point_idx] != new_grid_pos) {
            vertices[dragged_point_idx] = new_grid_pos;
            value_changed = true;
        }
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        dragged_point_idx = -1;
    }

    if (vertices.size() >= 2) {
        for (size_t i = 0; i < vertices.size() - 1; ++i) {
            ImVec2 p1 = GridToScreen(vertices[i]);
            ImVec2 p2 = GridToScreen(vertices[i + 1]);
            draw_list->AddLine(p1, p2, IM_COL32(0, 220, 255, 255), 2.0f);
        }
        ImVec2 p_last = GridToScreen(vertices.back());
        ImVec2 p_first = GridToScreen(vertices.front());
        draw_list->AddLine(p_last, p_first, IM_COL32(0, 220, 255, 100), 1.5f);
    }

    for (int i = 0; i < (int)vertices.size(); ++i) {
        ImVec2 node_pos = GridToScreen(vertices[i]);
        ImU32 col = (i == dragged_point_idx) ? IM_COL32(255, 255, 0, 255) : IM_COL32(255, 100, 100, 255);

        draw_list->AddCircleFilled(node_pos, node_radius, col);
        draw_list->AddCircle(node_pos, node_radius, IM_COL32(0, 0, 0, 255), 0, 1.5f);

        if (i == dragged_point_idx || (is_hovered && std::abs(io.MousePos.x - node_pos.x) < 10 && std::abs(io.MousePos.y - node_pos.y) < 10)) {
            char buf[32];
            snprintf(buf, sizeof(buf), "(%d, %d)", vertices[i].x, vertices[i].y);
            draw_list->AddText(ImVec2(node_pos.x + 8, node_pos.y - 12), IM_COL32(255, 255, 255, 255), buf);
        }
    }

    ImGui::TextDisabled("LMB: add/drag");
    ImGui::TextDisabled("RMB: remove");
    ImGui::TextDisabled("Ctrl+LMB: insert on edge");

    ImGui::PopID();
    return value_changed;
}