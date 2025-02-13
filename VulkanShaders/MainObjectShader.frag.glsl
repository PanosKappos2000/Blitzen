#version 450

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

#include "../VulkanShaderHeaders/ShaderBuffers.glsl"

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 tangent;
layout(location = 3) in flat uint materialTag;
layout(location = 4) in vec3 modelPos;

// The texture binding is only needed in the fragment shader
layout(set = 1, binding = 0) uniform sampler2D textures[];

// The color that will be calculated for the current fragment
layout (location = 0) out vec4 outColor;

void main()
{
    // Get the material from the material buffer based on the material tag that was passed from the vertex shader
    Material material = materialBuffer.materials[materialTag];

    vec4 albedoMap = texture(textures[nonuniformEXT(material.albedoTag)], uv);
    
    vec3 normalMap = vec3(0, 0, 1);
    if(material.normalTag != 0)
        normalMap = texture(textures[nonuniformEXT(material.normalTag)], uv).rgb * 2 - 1;

    vec3 emissiveMap = vec3(0.0);
    if(material.albedoTag != 0)
        emissiveMap = texture(textures[nonuniformEXT(material.emissiveTag)], uv).rgb;

    vec3 bitangent = cross(normal, tangent.xyz) * tangent.w;
	vec3 nrm = normalize(normalMap.r * tangent.xyz + normalMap.g * bitangent + normalMap.b * normal);
	float ndotl = max(dot(nrm, normalize(vec3(-1, 1, -1))), 0.0);

    // The if statement is temporary
    if(materialTag != 0)
        outColor = albedoMap * sqrt(ndotl + 0.05);
    else
        outColor = vec4(normal * sqrt(ndotl + 0.05), 1);

    /*// Bad for performance and does not do anything for now, will be fixed later
    if(albedoMap.a < 0.5)
        discard;*/
}