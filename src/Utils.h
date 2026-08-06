#pragma once

#include <cstdint>
#include <string>
#include <glm/glm.hpp>

uint32_t DecodeUTF8(const char*& ptr);
std::string StripIcons(const char* utf8_str);

void RescaleVertices(
    std::vector<glm::i32vec2>& vertices,
    const glm::i32vec2& base_size,
    const glm::i32vec2& new_size);