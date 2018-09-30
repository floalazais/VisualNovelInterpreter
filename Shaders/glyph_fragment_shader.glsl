#version 330 core

in vec2 vertexTexCoord;

out vec4 fragmentColor;

uniform sampler2D glyphTextureId;
uniform vec3 textColor;
uniform float opacity;

void main()
{
    vec4 textureColor = vec4(textColor, texture(glyphTextureId, vec2(vertexTexCoord.x, vertexTexCoord.y)).r);
    fragmentColor = vec4(textureColor.xyz, textureColor.w * opacity);
}
