#version 330 core

in vec2 vertexTexCoord;

out vec4 fragmentColor;

uniform sampler2D glyphTextureId;
uniform vec3 textColor;

void main()
{
    fragmentColor = vec4(textColor, texture(glyphTextureId, vec2(vertexTexCoord.x, vertexTexCoord.y)).r);
}
