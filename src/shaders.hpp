#pragma once

#include "arena.hpp"

#include <glm/glm.hpp>
#include <glad/glad.h>

struct ShaderProgram
{
    GLuint program_id; // Make the paths part of the shader and perhaps initialize them passing the arena.
};

void InitShaders(ShaderProgram* shader_program, const char* vertex_shader_path, const char* fragmen_shader_path, Arena& arena);

void SetMat4(ShaderProgram* shader_program, const char* name, const glm::mat4& matrix);

