#pragma once

#include <cstdint>
#include <string>

uint32_t DecodeUTF8(const char*& ptr);
std::string StripIcons(const char* utf8_str);