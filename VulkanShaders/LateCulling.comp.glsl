#version 450

#extension GL_GOOGLE_include_directive : require

#define COMPUTE_PIPELINE

#include "ShaderBuffers.glsl.h"

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout (set = 0, binding = 2) uniform CullingData
{
    vec4 frustumPlanes[6];

    float proj0;
    float proj5;
    float zNear;

    // Occulusion culling depth pyramid data
    float pyramidWidth;
    float pyramidHeight;

    uint occlusion;
    uint lod;
}cullingData;

layout (set = 0, binding = 3) uniform sampler2D depthPyramid;

// 2D Polyhedral Bounds of a Clipped, Perspective-Projected 3D Sphere. Michael Mara, Morgan McGuire. 2013
bool projectSphere(vec3 C, float r, float znear, float P00, float P11, out vec4 aabb)
{
	if (C.z < r + znear)
		return false;

	vec2 cx = -C.xz;
	vec2 vx = vec2(sqrt(dot(cx, cx) - r * r), r) / length(cx);

	vec2 minx = mat2(vx.x, vx.y, -vx.y, vx.x) * cx;
	vec2 maxx = mat2(vx.x, -vx.y, vx.y, vx.x) * cx;

	vec2 cy = -C.yz;
	vec2 vy = vec2(sqrt(dot(cy, cy) - r * r), r) / length(cy);

	vec2 miny = mat2(vy.x, -vy.y, vy.y, vy.x) * cy;
	vec2 maxy = mat2(vy.x, vy.y, -vy.y, vy.x) * cy;

	aabb = vec4(minx.x / minx.y * P00, miny.x / miny.y * P11, maxx.x / maxx.y * P00, maxy.x / maxy.y * P11) 
    * vec4(0.5f, -0.5f, 0.5f, -0.5f) + vec4(0.5f);

	return true;
}

void main()
{
    uint objectIndex = gl_GlobalInvocationID.x;

    // Access the object's data
    RenderObject currentObject = bufferAddrs.objectBuffer.objects[objectIndex];
    MeshInstance currentInstance = bufferAddrs.meshInstanceBuffer.instances[currentObject.meshInstanceId];
    Surface currentSurface = bufferAddrs.surfaceBuffer.surfaces[currentObject.surfaceId];

    vec3 center = RotateQuat(currentSurface.center, currentInstance.orientation) * currentInstance.scale  + currentInstance.pos;
    center = (shaderData.view * vec4(center, 1)).xyz;
    float radius = currentSurface.radius * currentInstance.scale;
    bool visible = true;
    for(uint i = 0; i < 6; ++i)
    {
        visible = visible && dot(cullingData.frustumPlanes[i], vec4(center, 1)) > - radius;
    }

    if (visible && uint(cullingData.occlusion) == 1)
	{
		vec4 aabb;
		if (projectSphere(center, radius, cullingData.zNear, cullingData.proj0, cullingData.proj5, aabb))
		{
			float width = (aabb.z - aabb.x) * cullingData.pyramidWidth;
			float height = (aabb.w - aabb.y) * cullingData.pyramidHeight;

            // Find the mip map level that will match the screen size of the sphere
			float level = floor(log2(max(width, height)));
			float depth = textureLod(depthPyramid, (aabb.xy + aabb.zw) * 0.5, level).x;
			float depthSphere = cullingData.zNear / (center.z - radius);
			visible = visible && depthSphere > depth;
		}
	}

    // Only add the object to the indirect buffer if it was invisible last frame but visible now
    uint currentVisibility = bufferAddrs.visibilityBuffer.visibilities[objectIndex];
    if(visible && currentVisibility == 0)
    {
        // With each element that is added to the draw list, increment the count
        uint drawIndex = atomicAdd(bufferAddrs.indirectCount.drawCount, 1);

        // The level of detail index that should be used is derived by the distance fromt he camera
        float lodDistance = log2(max(1, (distance(center, vec3(0)) - radius)));
	    uint lodIndex = clamp(int(lodDistance), 0, int(currentSurface.lodCount) - 1);

        MeshLod currentLod = cullingData.lod == 1 ? currentSurface.lod[lodIndex] : currentSurface.lod[0];

        // The object index is needed to know which element to access in the per object data buffer
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].objectId = objectIndex;

        // Indirect draw commands
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].indexCount = currentLod.indexCount;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].instanceCount = 1;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].firstIndex = currentLod.firstIndex;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].vertexOffset = currentSurface.vertexOffset;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].firstInstance = 0;
    }

    // Tell the next frame if the object was visible last frame
    bufferAddrs.visibilityBuffer.visibilities[objectIndex] = visible ? 1 : 0;
}