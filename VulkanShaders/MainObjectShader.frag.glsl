#version 450

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

#include "ShaderBuffers.glsl.h"

layout (location = 0) in vec2 uvMap;
layout (location = 1) in flat uint materialTag;
layout (location = 2) in vec3 normal;

layout (location = 0) out vec4 finalColor;

layout(set = 1, binding = 0) uniform sampler2D textures[];

void main()
{
    Material currentMaterial = shaderData.materialBuffer.materials[materialTag];

    float diffuseFactor = max(dot(normal, -shaderData.sunDir), 0.0);
    vec4 diffuseSampler = texture(textures[currentMaterial.diffuseMapIndex], uvMap);
    vec4 diffuse = vec4(vec3(shaderData.sunColor * diffuseFactor), diffuseSampler.a);
    diffuse *= diffuseSampler;
    finalColor = diffuse + currentMaterial.diffuseColor;
}