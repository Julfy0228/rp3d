#include "Utils.h"

#include <IconsLucide.h>
#include <cmath>

uint32_t DecodeUTF8(const char*& ptr)
{
    const unsigned char* p = reinterpret_cast<const unsigned char*>(ptr);
    uint32_t c = *p;
    if (c < 0x80) { ptr += 1; return c; }
    if ((c & 0xE0) == 0xC0) { uint32_t res = ((c & 0x1F) << 6) | (p[1] & 0x3F); ptr += 2; return res; }
    if ((c & 0xF0) == 0xE0) { uint32_t res = ((c & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); ptr += 3; return res; }
    if ((c & 0xF8) == 0xF0) { uint32_t res = ((c & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F); ptr += 4; return res; }
    ptr += 1;
    return c;
}

std::string StripIcons(const char* utf8_str)
{
    if (!utf8_str) return "";

    std::string clean_str;
    const char* ptr = utf8_str;

    while (*ptr)
    {
        const char* prev_ptr = ptr;
        uint32_t codepoint = DecodeUTF8(ptr);

        if (codepoint >= ICON_MIN_LC && codepoint <= ICON_MAX_16_LC)
        {
            if (*ptr == ' ')
                ptr++;
            continue;
        }

        clean_str.append(prev_ptr, ptr - prev_ptr);
    }

    return clean_str;
}

void RescaleVertices(
    std::vector<glm::i32vec2>& vertices,
    const glm::i32vec2& base_size,
    const glm::i32vec2& new_size)
{
    if (base_size.x <= 0 || base_size.y <= 0 || vertices.empty())
        return;

    glm::i32vec2 min_v = vertices[0];
    for (const auto& v : vertices)
    {
        min_v.x = std::min(min_v.x, v.x);
        min_v.y = std::min(min_v.y, v.y);
    }

    double scale_x = static_cast<double>(new_size.x) / static_cast<double>(base_size.x);
    double scale_y = static_cast<double>(new_size.y) / static_cast<double>(base_size.y);

    for (auto& v : vertices)
    {
        double rel_x = v.x - min_v.x;
        double rel_y = v.y - min_v.y;
        v.x = min_v.x + static_cast<int>(std::lround(rel_x * scale_x));
        v.y = min_v.y + static_cast<int>(std::lround(rel_y * scale_y));
    }
}