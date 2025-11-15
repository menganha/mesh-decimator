#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

uniform mat4 modelview;
uniform mat4 projection;

out vec4 color;

void main()
{
    gl_Position = projection * modelview * vec4(aPos, 1.0);
    color = vec4(aColor, 1.0);
}
