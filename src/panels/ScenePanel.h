#pragma once

#include <Scene.h>

class ViewportRenderer;

class ScenePanel
{
public:
    void draw(Scene& scene, ViewportRenderer* viewport_renderer);
};