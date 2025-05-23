#version 460

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

//#define MESH_TEST
//#define RAYTRACING

#ifdef RAYTRACING
#extension GL_EXT_ray_query: require
layout(set = 0, binding = 15) uniform accelerationStructureEXT tlas;
#endif

#include "../VulkanShaderHeaders/ShaderBuffers.glsl"

// Specialization constant. Its value changes for the post pass pipeline. This should theoritically allow for gpu compiler optimizations
layout (constant_id = 0) const uint POST_PASS = 0;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 tangent;
layout(location = 3) in flat uint materialTag;
layout(location = 4) in vec3 modelPos;

// The texture binding is only needed in the fragment shader
layout(set = 1, binding = 0) uniform sampler2D textures[];

// The color that will be calculated for the current fragment
layout (location = 0) out vec4 outColor;

#ifndef MESH_TEST
void main()
{
    // Get the material from the material buffer based on the material tag that was passed from the vertex shader
    Material material = materialBuffer.materials[materialTag];

    vec4 albedoMap = vec4(0.5f, 0.5f, 0.5f, 1);
    if(material.albedoTag != 0)
    {
        albedoMap = texture(textures[nonuniformEXT(material.albedoTag)], uv);
    }
    
    vec3 normalMap = vec3(0, 0, 1);
    if(material.normalTag != 0)
    {
        normalMap = texture(textures[nonuniformEXT(material.normalTag)], uv).rgb * 2 - 1;
    }

    vec3 emissiveMap = vec3(0.0);
    if(material.emissiveTag != 0)
    {
        emissiveMap = texture(textures[nonuniformEXT(material.emissiveTag)], uv).rgb;
    }

    vec3 bitangent = cross(normal, tangent.xyz) * tangent.w;
    vec3 finalTangent = tangent.xyz - dot(tangent.xyz, normal) * normal;
	vec3 nrm = normalize(normalMap.r * finalTangent + normalMap.g * bitangent + normalMap.b * normal);
    vec3 sunDirection = normalize(vec3(-1, 1, -1));
	float ndotl = max(dot(nrm, sunDirection), 0.0);

    #ifdef RAYTRACING
	    rayQueryEXT rayQuery;
        uint rayflags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsCullNoOpaqueEXT;
	    rayQueryInitializeEXT(rayQuery, tlas, rayflags, 0xff, modelPos, 1e-2f, sunDirection, 100);
	    rayQueryProceedEXT(rayQuery);
	    ndotl *= (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT) ? 1.0 : 0.1;
    #endif

    outColor = vec4(albedoMap.rgb * sqrt(ndotl + 0.05) + emissiveMap, albedoMap.a);

    if(POST_PASS != 0 && albedoMap.a < 0.5)
    {
        discard;
    }
}
#else
void main()
{
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
#endif