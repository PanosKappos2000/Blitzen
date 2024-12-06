#version 450

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

#include "ShaderBuffers.glsl.h"

layout (location = 0) in vec2 uvMap;
layout (location = 1) in flat uint materialTag;

layout (location = 0) out vec4 finalColor;

layout(set = 1, binding = 0) uniform sampler2D textures[];

void main()
{
    Material currentMaterial = shaderData.materialBuffer.materials[materialTag];

    vec3 color = currentMaterial.diffuseColor.xyz * texture(textures[currentMaterial.diffuseMapIndex], uvMap).xyz;
    finalColor = vec4(color, 1.0);
}