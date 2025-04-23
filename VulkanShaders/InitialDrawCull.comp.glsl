#version 450
#extension GL_GOOGLE_include_directive : require
//#extension GL_EXT_debug_printf : enable
#define COMPUTE_PIPELINE
#include "../VulkanShaderHeaders/ShaderBuffers.glsl"
#include "../VulkanShaderHeaders/CullingShaderData.glsl"

#define CULL  true

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout (push_constant) uniform CullingConstants
{
    RenderObjectBuffer renderObjectBuffer;
    uint drawCount;
}pushConstant;

void main()
{
    // The object index is for the current object's element in the render object
	uint objectIndex = gl_GlobalInvocationID.x;
    if(pushConstant.drawCount <= objectIndex)
    {
        return;
    }
    // This shader only processes objects that were visible last frame
    if(visibilityBuffer.visibilities[objectIndex] == 0)
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
	
    // If the object passed frustum culling, draw commands are created for it
    if(visible)
    {
        uint lodOffset = surfaceBuffer.surfaces[obj.surfaceId].lodOffset;
        uint lodCount = surfaceBuffer.surfaces[obj.surfaceId].lodCount;
        uint lodIndex = LODSelection(center, radius, transform.scale, viewData.lodTarget, lodOffset, lodCount);
        lodIndex += lodOffset;
        Lod currentLod = lodBuffer.levels[lodIndex];
        
        // Increments the draw count
        uint drawID = atomicAdd(indirectDrawCountBuffer.drawCount, 1);
        // The object index is needed to know which element to access in the per object data buffer
        indirectDrawBuffer.draws[drawID].objectId = objectIndex;
        indirectDrawBuffer.draws[drawID].indexCount = currentLod.indexCount;
        indirectDrawBuffer.draws[drawID].instanceCount = 1;
        indirectDrawBuffer.draws[drawID].firstIndex = currentLod.firstIndex;
        indirectDrawBuffer.draws[drawID].vertexOffset = 0;
        indirectDrawBuffer.draws[drawID].firstInstance = 0;
    } 
}