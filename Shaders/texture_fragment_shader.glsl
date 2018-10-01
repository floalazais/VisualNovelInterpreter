#version 330 core

in vec2 vertexTexCoord;

out vec4 fragmentColor;

uniform sampler2D textureId;
uniform float opacity;

void main()
{
    vec4 textureColor = texture(textureId, vec2(vertexTexCoord.x, vertexTexCoord.y));
    fragmentColor = vec4(textureColor.xyz, textureColor.w * opacity);
}
