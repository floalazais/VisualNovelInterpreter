#version 330 core

in vec2 vertexTexCoord;

out vec4 fragmentColor;

uniform vec3 color;
uniform float opacity;

void main()
{
    fragmentColor = vec4(color, opacity);
}
