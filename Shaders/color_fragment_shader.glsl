#version 330 core

in vec2 vertexTexCoord;

out vec4 fragmentColor;

uniform vec3 color;

void main()
{
    fragmentColor = vec4(color, 1.0f);
}
