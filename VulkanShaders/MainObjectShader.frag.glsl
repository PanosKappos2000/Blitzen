#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec2 uvMap;
layout (location = 1) in flat uint textureTag;

layout (location = 0) out vec4 finalColor;

layout(set = 1, binding = 0) uniform sampler2D textures[];

void main()
{
    vec3 color = texture(textures[textureTag], uvMap).xyz;
    finalColor = vec4(color, 1.0);
}