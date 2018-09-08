#version 330 core

layout (location = 0) in vec2 layoutPosition;
layout (location = 1) in vec2 layoutTexCoord;

uniform mat4 model;
uniform mat4 projection;

out vec2 vertexTexCoord;

void main()
{
    gl_Position = projection * model * vec4(layoutPosition, 0.0f, 1.0f);
    vertexTexCoord = layoutTexCoord;
}
