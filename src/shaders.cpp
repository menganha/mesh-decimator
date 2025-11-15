#include "shaders.hpp"

#include "log.hpp"

#include <cstdio>

void InitShaders(ShaderProgram* shader, const char* vertex_shader_path, const char* fragment_shader_path, Arena& arena)
{
    GLuint shader_program = glCreateProgram();
    shader->program_id = shader_program;

    int index = 0;
    while ( index < 2 )
    {
        // Create shader in open GL and compile
        GLuint      gl_shader;
        const char* shader_path;
        if ( index == 0 )
        {
            shader_path = vertex_shader_path;
            gl_shader = glCreateShader(GL_VERTEX_SHADER);
        }
        else
        {
            shader_path = fragment_shader_path;
            gl_shader = glCreateShader(GL_FRAGMENT_SHADER);
        }
        index++;

        // Read file
        FILE* file_handle = fopen(shader_path, "rb"); // TODO: Check if its null and raise error
        fseek(file_handle, 0, SEEK_END);
        long file_size = ftell(file_handle);
        rewind(file_handle);

        char* shader_string = arenaAlloc<char>(arena, file_size);
        fread(shader_string, 1, file_size, file_handle);
        fclose(file_handle);

        glShaderSource(gl_shader, 1, &shader_string, NULL);
        glCompileShader(gl_shader);

        // check for shader compile errors
        int  success;
        char infoLog[512]; // TODO Pass this to a temp arena. It looks ugly
        glGetShaderiv(gl_shader, GL_COMPILE_STATUS, &success);
        if ( !success )
        {
            glGetShaderInfoLog(gl_shader, 512, NULL, infoLog);
            LERROR("Shader compilation failed for shader %s:  %s", shader_path, infoLog);
        }
        glAttachShader(shader_program, gl_shader);
        glDeleteShader(gl_shader); // flagd for deletion
    }

    // link the shader program
    int  success;
    char infoLog[512];
    glLinkProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success); // check for linking errors
    if ( !success )
    {
        glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
        LERROR("Shader linking failed: %s", infoLog);
    }
}

void SetMat4(ShaderProgram* shader_program, const char* name, const glm::mat4& matrix)
{
    auto matrix_location = glGetUniformLocation(shader_program->program_id, name);
    glUniformMatrix4fv(matrix_location, 1, GL_FALSE, &matrix[0][0]);
}
