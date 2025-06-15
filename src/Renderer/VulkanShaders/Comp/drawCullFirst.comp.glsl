#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : enable

#define DOUBLE_PASS
#define COMPUTE_PIPELINE

#include "../Headers/sharedBuffers.glsl"
#include "../Headers/cullBuffers.glsl"
#include "../Headers/math.glsl"

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

void main()
{
	uint objectIndex = gl_GlobalInvocationID.x + pushConstant.drawOffset;
    if(pushConstant.drawCount <= objectIndex)
    {
        return;
    }

    // This shader only processes objects that were visible last frame
    if(rwssbo_DrawVis.data[objectIndex] == 0)
    {
        return;
    }

    RenderObject obj = ssbo_render.data[objectIndex];
    Transform transform = transformBuffer.instances[obj.meshInstanceId];

    // Frustum culling
    vec3 center;
	float radius;
	bool visible = CheckFrustum(center, radius, surfaceBuffer.surfaces[obj.surfaceId].center, surfaceBuffer.surfaces[obj.surfaceId].radius, transform.scale, transform.pos, transform.orientation,
        viewData.view, viewData.frustumRight, viewData.frustumLeft, viewData.frustumTop, viewData.frustumBottom, viewData.zNear, viewData.zFar);
	
    if(visible)
    {
        uint lodIndex = LODSelection(center, radius, transform.scale, viewData.lodTarget, surfaceBuffer.surfaces[obj.surfaceId].lodOffset, surfaceBuffer.surfaces[obj.surfaceId].lodCount);
        
        // Increments the draw count
        uint drawID = atomicAdd(indirectDrawCountBuffer.drawCount, 1);

        // Object id
        rwssbo_DrawCmd.data[drawID].objectId = objectIndex;

        rwssbo_DrawCmd.data[drawID].indexCount = ssbo_LODs.data[lodIndex].indexCount;
        rwssbo_DrawCmd.data[drawID].firstIndex = ssbo_LODs.data[lodIndex].firstIndex;
        rwssbo_DrawCmd.data[drawID].vertexOffset = 0;

        // instances
        rwssbo_DrawCmd.data[drawID].instanceCount = 1;
        rwssbo_DrawCmd.data[drawID].firstInstance = 0;
    } 
}