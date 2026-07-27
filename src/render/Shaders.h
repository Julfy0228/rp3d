#pragma once

#include <glad/glad.h>

namespace render
{
    bool load_shader_program(const char* vertex_path, const char* fragment_path, GLuint& out_program);
}