#version 450
#extension GL_GOOGLE_include_directive : require
//#extension GL_EXT_debug_printf : enable

#define COMPUTE_PIPELINE
#include "../VulkanShaderHeaders/ShaderBuffers.glsl"
#include "../VulkanShaderHeaders/CullingShaderData.glsl"
#define CULL  true

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
//layout (set = 0, binding = 3) uniform sampler2D depthPyramid;

void main()
{
    uint objectIndex = gl_GlobalInvocationID.x;
    if(pushConstant.drawCount <= objectIndex)
    {
        return;
    }
    RenderObject obj = pushConstant.renderObjectBuffer.objects[objectIndex];
    Transform transform = transformBuffer.instances[obj.meshInstanceId];
    
    // Frustum culling
    vec3 center;
	float radius;
	bool visible = IsObjectInsideViewFrustum(center, radius, 
        surfaceBuffer.surfaces[obj.surfaceId].center, surfaceBuffer.surfaces[obj.surfaceId].radius, // bounding sphere
        transform.scale, transform.pos, transform.orientation, // object transform
        viewData.view, // view matrix
        viewData.frustumRight, viewData.frustumLeft, // frustum planes
        viewData.frustumTop, viewData.frustumBottom, // frustum planes part 2
        viewData.zNear, viewData.zFar // zFar and zNear
    );

    // TODO: Remember to enable this for testing
    // If an object passes frustum culling, it goes through occlusion culling
    /*if (false)
	{
		vec4 aabb;
		if (projectSphere(center, radius, viewData.zNear, viewData.proj0, viewData.proj5, aabb))
		{
			visible = visible && OcclusionCullingPassed(aabb, depthPyramid, viewData.pyramidWidth, viewData.pyramidHeight, center, radius, viewData.zNear);
		}
	}*/

    // Draw commands assigned if the object is visible
    if(visible)
    {
        uint lodOffset = surfaceBuffer.surfaces[obj.surfaceId].lodOffset;
        uint lodCount = surfaceBuffer.surfaces[obj.surfaceId].lodCount;
        uint lodIndex = LODSelection(center, radius, transform.scale, viewData.lodTarget, lodOffset, lodCount);
        lodIndex += lodOffset;
        Lod currentLod = lodBuffer.levels[lodIndex];

        // Increments draw command count
        uint drawID = atomicAdd(indirectDrawCountBuffer.drawCount, 1);
        // Indirect commands + object id (the object id is needed for the vertex shader to access object data)
        indirectDrawBuffer.draws[drawID].objectId = objectIndex;
        indirectDrawBuffer.draws[drawID].indexCount = currentLod.indexCount;
        indirectDrawBuffer.draws[drawID].instanceCount = 1;
        indirectDrawBuffer.draws[drawID].firstIndex = currentLod.firstIndex;
        indirectDrawBuffer.draws[drawID].vertexOffset = 0;
        indirectDrawBuffer.draws[drawID].firstInstance = 0;
    }
}