#version 450

#extension GL_GOOGLE_include_directive : require

#define COMPUTE_PIPELINE

#include "../VulkanShaderHeaders/ShaderBuffers.glsl"

#define CULL  true
#define LOD_BASE    10.f
#define LOD_STEP    1.5f

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout (set = 0, binding = 2) uniform CullingData
{
    // frustum planes
    float frustumRight;
    float frustumLeft;
    float frustumTop;
    float frustumBottom;

    float proj0;// The 1st element of the projection matrix
    float proj5;// The 12th element of the projection matrix

    // The draw distance and zNear, needed for both occlusion and frustum culling
    float zNear;
    float zFar;

    // Occulusion culling depth pyramid data
    float pyramidWidth;
    float pyramidHeight;

    float lodTarget;

    // Debug values
    uint occlusionEnabled;
    uint lodEnabled;

    uint drawCount;
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

    if(cullingData.drawCount < objectIndex)
        return;

    // Access the object's data
    RenderObject currentObject = bufferAddrs.objectBuffer.objects[objectIndex];
    MeshInstance currentInstance = bufferAddrs.transformBuffer.instances[currentObject.meshInstanceId];
    Surface currentSurface = bufferAddrs.surfaceBuffer.surfaces[currentObject.surfaceId];

    // Promote the sphere center to view coordinates
    vec3 center = RotateQuat(currentSurface.center, currentInstance.orientation) * currentInstance.scale + currentInstance.pos;
    center = (shaderData.view * vec4(center, 1)).xyz;

    // Scale the sphere radius
	float radius = currentSurface.radius * currentInstance.scale;

    // Check that the bounding sphere is inside the view frustum(frustum culling)
	bool visible = true;
    // the left/top/right/bottom plane culling utilizes frustum symmetry to cull against two planes at the same time(Arseny Kapoulkine)
    visible = visible && center.z * cullingData.frustumLeft - abs(center.x) * cullingData.frustumRight > -radius;
	visible = visible && center.z * cullingData.frustumBottom - abs(center.y) * cullingData.frustumTop > -radius;
	// the near/far plane culling uses camera space Z directly
	visible = visible && center.z + radius > cullingData.zNear && center.z - radius < cullingData.zFar;

    // Occlusion culling
    if (visible && uint(cullingData.occlusionEnabled) == 1)
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

        // The lod index is declared here. if LODs are not enabled the most detailed version of an object will be used by default
        uint lodIndex = 0;
        /*  
            The LOD index is calculated using a formula where the distance to bounding sphere
            surface is taken and the minimum error that would result in acceptable
            screen-space deviation is computed based on camera parameters
        */
        if (cullingData.lodEnabled == 1)
		{
			float distance = max(length(center) - radius, 0);
			float threshold = distance * cullingData.lodTarget / currentInstance.scale;
			for (uint i = 1; i < currentSurface.lodCount; ++i)
				if (currentSurface.lod[i].error < threshold)
					lodIndex = i;
		}
        MeshLod currentLod = currentSurface.lod[lodIndex];

        // The object index is needed to know which element to access in the per object data buffer
        bufferAddrs.indirectDrawBuffer.draws[drawIndex].objectId = objectIndex;

        // Indirect draw commands
        bufferAddrs.indirectDrawBuffer.draws[drawIndex].indexCount = currentLod.indexCount;
        bufferAddrs.indirectDrawBuffer.draws[drawIndex].instanceCount = 1;
        bufferAddrs.indirectDrawBuffer.draws[drawIndex].firstIndex = currentLod.firstIndex;
        bufferAddrs.indirectDrawBuffer.draws[drawIndex].vertexOffset = currentSurface.vertexOffset;
        bufferAddrs.indirectDrawBuffer.draws[drawIndex].firstInstance = 0;

        // Indirect task commands
        bufferAddrs.indirectTaskBuffer.tasks[drawIndex].taskId = currentLod.firstMeshlet;
        bufferAddrs.indirectTaskBuffer.tasks[drawIndex].groupCountX = (currentLod.meshletCount + 31) / 32;
        bufferAddrs.indirectTaskBuffer.tasks[drawIndex].groupCountY = 1;
        bufferAddrs.indirectTaskBuffer.tasks[drawIndex].groupCountZ = 1;

    }

    // Tell the next frame if the object was visible last frame
    bufferAddrs.visibilityBuffer.visibilities[objectIndex] = visible ? 1 : 0;
}