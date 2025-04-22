#version 450
#extension GL_GOOGLE_include_directive : require
//#extension GL_EXT_debug_printf : require
#define COMPUTE_PIPELINE
#include "../VulkanShaderHeaders/ShaderBuffers.glsl"
#include "../VulkanShaderHeaders/CullingShaderData.glsl"

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

void main()
{
    // The object index is for the current object's element in the render object
	uint objectIndex = gl_GlobalInvocationID.x;
    if(cullPC.drawCount <= objectIndex)
    {
        return;
    }
    RenderObject obj = onpcReflectiveObjectBuffer.objects[objectIndex];
    Transform transform = transformBuffer.instances[obj.meshInstanceId];
    Surface surface = surfaceBuffer.surfaces[obj.surfaceId];

    vec3 center;
    float radius;
    bool visible = IsObjectInsideViewFrustum(
        center, radius, surface.center, surface.radius, 
        transform.scale, transform.pos, transform.orientation, 
        viewData.view, 
        viewData.frustumRight, viewData.frustumLeft, 
        viewData.frustumTop, viewData.frustumBottom,
        viewData.zNear, viewData.zFar
    );

    // Create draw commands for the objects that passed frustum culling
    if(visible)
    {
        
        uint lodIndex = LODSelection(center, radius, transform.scale, viewData.lodTarget, 
            surfaceBuffer.surfaces[obj.surfaceId].lodOffset, surfaceBuffer.surfaces[obj.surfaceId].lodCount);
        Lod currentLod = lodBuffer.levels[lodIndex];

        // Increases draw count
        uint drawID = atomicAdd(indirectDrawCountBuffer.drawCount, 1);
        // Draw commands + object id
        indirectDrawBuffer.draws[drawID].objectId = objectIndex;
        indirectDrawBuffer.draws[drawID].indexCount = currentLod.indexCount;
        indirectDrawBuffer.draws[drawID].instanceCount = 1;
        indirectDrawBuffer.draws[drawID].firstIndex = currentLod.firstIndex;
        indirectDrawBuffer.draws[drawID].vertexOffset = 0;
        indirectDrawBuffer.draws[drawID].firstInstance = 0;
    }
}