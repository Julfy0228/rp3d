#pragma once

#include <Scene.h>
#include <memory>

class Renderer;

class ScenePanel
{
public:
    ScenePanel();
    ~ScenePanel();

    bool init();
    void shutdown();
    void draw(Scene& scene);

private:
    std::unique_ptr<Renderer> renderer;
};