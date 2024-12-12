#version 450

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

#include "ShaderBuffers.glsl.h"

layout (location = 0) in vec2 uvMap;
layout (location = 1) in vec3 normal;
layout (location = 2) in flat uint materialTag;
layout (location = 3) in vec3 viewPosition;
layout (location = 4) in vec3 modelPosition;

layout (location = 0) out vec4 finalColor;

layout(set = 1, binding = 0) uniform sampler2D textures[];

void main()
{
    Material currentMaterial = bufferAddrs.materialBuffer.materials[materialTag];

    float diffuseFactor = max(dot(normal, -shaderData.sunDir), 0.0);
    vec4 diffuseSampler = texture(textures[currentMaterial.diffuseMapIndex], uvMap);
    vec4 diffuse = vec4(vec3(shaderData.sunColor * diffuseFactor), diffuseSampler.a);
    diffuse *= diffuseSampler;

    vec3 viewDirection = normalize(viewPosition - modelPosition);
    vec3 halfDirection = normalize(viewDirection - shaderData.sunDir);
    float specularFactor = pow(max(dot(halfDirection, normal), 0), currentMaterial.shininess);
    vec4 specularSampler = texture(textures[currentMaterial.specularMapIndex], uvMap);
    vec4 specular = vec4(vec3(shaderData.sunColor * specularFactor), specularSampler.a);
    specular *= specularSampler;

    finalColor = diffuseSampler;
}