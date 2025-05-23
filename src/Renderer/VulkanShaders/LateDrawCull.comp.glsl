#version 450
#extension GL_GOOGLE_include_directive : require

#define COMPUTE_PIPELINE
#include "../VulkanShaderHeaders/ShaderBuffers.glsl"
#include "../VulkanShaderHeaders/CullingShaderData.glsl"
#define CULL  true

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout (set = 0, binding = 3) uniform sampler2D depthPyramid;

void main()
{
    uint objectIndex = gl_GlobalInvocationID.x;

    // This a guard so that the compute shader does not go over the draw count
    if(pushConstant.drawCount <= objectIndex)
    {
        return;
    }

    // Access the object's data
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

    // If an object passes frustum culling, it goes through occlusion culling
    if (visible)
	{
		vec4 aabb;
		if (projectSphere(center, radius, viewData.zNear, viewData.proj0, viewData.proj5, aabb))
		{
			visible = visible && OcclusionCullingPassed(aabb, depthPyramid, viewData.pyramidWidth, viewData.pyramidHeight, center, radius, viewData.zNear);
		}
	}

    // Prepares draw commands if the object passed frustum and occlusion AND was not visible last frame OR is transparent
    // Transparent objects are only processed by this culling shader so last frame visibility is irrelevant
    if(visible && visibilityBuffer.visibilities[objectIndex] == 0)
    {
        uint lodOffset = surfaceBuffer.surfaces[obj.surfaceId].lodOffset;
        uint lodCount = surfaceBuffer.surfaces[obj.surfaceId].lodCount;
        uint lodIndex = LODSelection(center, radius, transform.scale, viewData.lodTarget, lodOffset, lodCount);
        lodIndex += lodOffset;
        Lod currentLod = lodBuffer.levels[lodIndex];

        // Increments draw count
        uint drawID = atomicAdd(indirectDrawCountBuffer.drawCount, 1);
        // Passes draw commands and object id
        indirectDrawBuffer.draws[drawID].objectId = objectIndex;
        indirectDrawBuffer.draws[drawID].indexCount = currentLod.indexCount;
        indirectDrawBuffer.draws[drawID].instanceCount = 1;
        indirectDrawBuffer.draws[drawID].firstIndex = currentLod.firstIndex;
        indirectDrawBuffer.draws[drawID].vertexOffset = 0;
        indirectDrawBuffer.draws[drawID].firstInstance = 0;
    }

    // Save the current frame visibility for this object
    visibilityBuffer.visibilities[objectIndex] = visible ? 1 : 0;
}