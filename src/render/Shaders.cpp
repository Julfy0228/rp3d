#include "Shaders.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
    std::string read_text_file(const char* path)
    {
        std::ifstream file(path, std::ios::in | std::ios::binary);
        if (!file)
            return {};

        std::ostringstream stream;
        stream << file.rdbuf();
        return stream.str();
    }

    GLuint compile_shader(GLenum shader_type, const std::string& source)
    {
        GLuint shader = glCreateShader(shader_type);
        const char* source_ptr = source.c_str();
        glShaderSource(shader, 1, &source_ptr, nullptr);
        glCompileShader(shader);

        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == GL_TRUE)
            return shader;

        GLint log_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        std::string log(static_cast<size_t>(log_length), '\0');
        glGetShaderInfoLog(shader, log_length, nullptr, log.data());
        std::cerr << "Shader compilation failed: " << log << std::endl;
        glDeleteShader(shader);
        return 0;
    }
}

bool render::load_shader_program(const char* vertex_path, const char* fragment_path, GLuint& out_program)
{
    std::string vertex_source = read_text_file(vertex_path);
    std::string fragment_source = read_text_file(fragment_path);
    if (vertex_source.empty() || fragment_source.empty())
    {
        std::cerr << "Failed to read shader sources" << std::endl;
        return false;
    }

    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_source);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
    if (vertex_shader == 0 || fragment_shader == 0)
    {
        if (vertex_shader != 0)
            glDeleteShader(vertex_shader);
        if (fragment_shader != 0)
            glDeleteShader(fragment_shader);
        return false;
    }

    out_program = glCreateProgram();
    glAttachShader(out_program, vertex_shader);
    glAttachShader(out_program, fragment_shader);
    glLinkProgram(out_program);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    GLint success = GL_FALSE;
    glGetProgramiv(out_program, GL_LINK_STATUS, &success);
    if (success == GL_TRUE)
        return true;

    GLint log_length = 0;
    glGetProgramiv(out_program, GL_INFO_LOG_LENGTH, &log_length);
    std::string log(static_cast<size_t>(log_length), '\0');
    glGetProgramInfoLog(out_program, log_length, nullptr, log.data());
    std::cerr << "Program link failed: " << log << std::endl;
    glDeleteProgram(out_program);
    out_program = 0;
    return false;
}