cmake_minimum_required(VERSION 3.14)
project(VoxelModeler LANGUAGES C CXX)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.3
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    earcuthpp
    GIT_REPOSITORY https://github.com/mapbox/earcut.hpp.git
    GIT_TAG        v2.2.4
)
FetchContent_GetProperties(earcuthpp)
if(NOT earcuthpp_POPULATED)
    FetchContent_Populate(earcuthpp)
endif()

set(APP_SOURCES
    src/main.cpp
    src/EditorApp.cpp
    src/Utils.cpp
    src/UndoManager.cpp
    src/panels/PropertiesPanel.cpp
    src/panels/ObjectsPanel.cpp
    src/panels/ScenePanel.cpp
    src/panels/StylePanel.cpp
    src/ViewportRenderer.cpp
    src/Widgets.cpp
    src/Native.cpp
)

set(EXTERNAL_SOURCES
    external/glad/glad.c
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_stdlib.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
    external/ImGuizmo/ImGuizmo.cpp
)

find_package(OpenGL REQUIRED)

add_executable(${PROJECT_NAME} ${APP_SOURCES} ${EXTERNAL_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    external/glad/include
    external/imgui
    external/imgui/backends
    external/ImGuizmo
    external/icons
    ${earcuthpp_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    glfw 
    glm
    ${OPENGL_LIBRARIES}
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${OPENGL_INCLUDE_DIR}
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)