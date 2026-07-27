#include "EditorApp.h"
#include "panels/PropertiesPanel.h"
#include "panels/ObjectsPanel.h"
#include "panels/ScenePanel.h"
#include "panels/StylePanel.h"
#include "ViewportRenderer.h"
#include "Native.h"
#include "Utils.h"

#include <iostream>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <ImGuizmo.h>
#include <IconsLucide.h>

void glfw_error_callback(int error, const char* description)
{
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

bool EditorApp::init()
{
    if (!init_window()) return false;

    init_imgui();
    objects_panel = new ObjectsPanel();
    scene_panel = new ScenePanel();
    style_panel = new StylePanel();
    properties_panel = new PropertiesPanel();
    viewport_renderer = new ViewportRenderer();
    if (!viewport_renderer->init()) return false;

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

    window = glfwCreateWindow(1280, 720, "VoxelModeler", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return false;
    }

    SetDarkTitlebar(window);
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
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    
    ImFontConfig font_config;
    font_config.OversampleH = 2;
    font_config.OversampleV = 2;
    io.Fonts->AddFontFromFileTTF(
        "assets/fonts/JetBrainsMono-Regular.ttf",
        20.0f,
        &font_config,
        io.Fonts->GetGlyphRangesCyrillic()
    );

    ImFontConfig icons_config;
    icons_config.OversampleH = 2;
    icons_config.OversampleV = 2;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = 20.0f;
    icons_config.GlyphOffset.y = 3.45f;
    static const ImWchar icon_ranges[] = { ICON_MIN_LC, ICON_MAX_16_LC, 0 };
    io.Fonts->AddFontFromFileTTF(
        "assets/fonts/lucide.ttf",
        18.0f,
        &icons_config,
        icon_ranges
    );

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    static void(*o_SetWindowTitle)(ImGuiViewport* vp, const char* title) = platform_io.Platform_SetWindowTitle;
    if (o_SetWindowTitle != nullptr)
    {
        platform_io.Platform_SetWindowTitle = [](ImGuiViewport* vp, const char* title)
        {
            std::string clean_title = StripIcons(title);
            o_SetWindowTitle(vp, clean_title.c_str());
        };
    }
}

void EditorApp::run()
{
    ImGuiIO& io = ImGui::GetIO();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false))
            undo_manager.undo(scene);
        if (io.KeyCtrl && (ImGui::IsKeyPressed(ImGuiKey_Y, false) || (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z, false))))
            undo_manager.redo(scene);

        draw();

        ImGui::Render();
        int w = 0, h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        auto clearColor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }
}

void EditorApp::draw()
{
    draw_dockspace();
    draw_panels();
}

void EditorApp::draw_panels()
{
    if (objects_panel)
        objects_panel->draw(scene, &undo_manager);

    if (properties_panel)
        properties_panel->draw(scene, &undo_manager);
    
    if (scene_panel)
        scene_panel->draw(scene, viewport_renderer);

    if (style_panel)
        style_panel->draw();
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
        const bool can_undo = undo_manager.can_undo();
        const bool can_redo = undo_manager.can_redo();

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Close", "Ctrl+W"))
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Scene"))
        {
            if (ImGui::MenuItem("Undo", "Ctrl+Z"))
                undo_manager.undo(scene);
            if (ImGui::MenuItem("Redo", "Ctrl+Y"))
                undo_manager.redo(scene);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Misc"))
        {
            if (ImGui::MenuItem("Style editor"))
                style_panel->show = !style_panel->show;
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
}

EditorApp::~EditorApp()
{
    shutdown();
}

void EditorApp::shutdown()
{
    if (objects_panel)
    {
        delete objects_panel;
        objects_panel = nullptr;
    }

    if (scene_panel)
    {
        delete scene_panel;
        scene_panel = nullptr;
    }

    if (properties_panel)
    {
        delete properties_panel;
        properties_panel = nullptr;
    }

    if (viewport_renderer)
    {
        viewport_renderer->shutdown();
        delete viewport_renderer;
        viewport_renderer = nullptr;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (window)
    {
        glfwDestroyWindow(window);
        window = nullptr;
    }

    glfwTerminate();
}
