#version 450

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

#include "../VulkanShaderHeaders/ShaderBuffers.glsl"

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in flat uint materialTag;
layout(location = 4) in vec3 modelPos;

layout(set = 1, binding = 0) uniform sampler2D textures[];

layout (location = 0) out vec4 finalColor;

void main()
{
    // Get the material from the material buffer based on the material tag that was passed from the vertex shader
    Material material = bufferAddrs.materialBuffer.materials[materialTag];

    vec4 diffuseSampler = texture(textures[nonuniformEXT(material.albedoTag)], uv);

    // Not optimal but I am going to keep this for the moment
    if(materialTag != 0)
        finalColor = diffuseSampler;
    else
        finalColor = vec4(normal, 1);
}