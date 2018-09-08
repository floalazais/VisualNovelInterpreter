#version 330 core

in vec2 vertexTexCoord;

out vec4 fragmentColor;

uniform sampler2D textureId;

void main()
{
    fragmentColor = texture(textureId, vec2(vertexTexCoord.x, vertexTexCoord.y));
}
