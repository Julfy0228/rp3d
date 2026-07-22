#pragma once

#include <Scene.h>

#include <vector>

class PropertiesPanel
{
private:
    void collect_selected(SceneNode* node, std::vector<SceneNode*>& selected_nodes);

public:
    void draw(Scene& scene);
};