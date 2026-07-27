#include <imgui.h>
#include <glm/glm.hpp>

namespace Colors
{
    inline constexpr ImVec4 TextDanger        = ImVec4(0.92f, 0.34f, 0.34f, 1.00f);

    inline constexpr ImVec4 GridLine          = ImVec4(0.55f, 0.58f, 0.62f, 0.30f);

    inline constexpr ImVec4 PlotLine          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    inline constexpr ImVec4 PlotLineClosing   = ImVec4(0.26f, 0.59f, 0.98f, 0.45f);

    inline constexpr ImVec4 PlotPoint         = ImVec4(0.92f, 0.34f, 0.34f, 1.00f);
    inline constexpr ImVec4 PlotPointHovered  = ImVec4(0.92f, 0.58f, 0.20f, 1.00f);
    inline constexpr ImVec4 PlotPointActive   = ImVec4(0.30f, 0.85f, 0.39f, 1.00f);

    inline constexpr ImVec4 GeomCenterA       = ImVec4(0.95f, 0.78f, 0.20f, 1.00f);
    inline constexpr ImVec4 GeomCenterB       = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);

    inline constexpr glm::vec4 ViewportGridMinor = glm::vec4(0.40f, 0.42f, 0.48f, 0.45f);
    inline constexpr glm::vec4 ViewportGridMajor = glm::vec4(0.58f, 0.60f, 0.68f, 0.70f);
}