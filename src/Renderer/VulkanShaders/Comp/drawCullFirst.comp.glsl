#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : enable

#define DOUBLE_PASS
#define COMPUTE_PIPELINE

#include "../Headers/sharedBuffers.glsl"
#include "../Headers/cullBuffers.glsl"
#include "../Headers/math.glsl"
#include "../Headers/bufferOffsets.glsl"

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

void main()
{
	uint objectIndex = gl_GlobalInvocationID.x + BLIT_OPAQUE_STATIC_RENDER_OFFSET;
    if(pushConstant.drawCount + BLIT_OPAQUE_STATIC_RENDER_OFFSET <= objectIndex)
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

    // Bounding sphere to world coordinates
    vec3 center = (viewData.view * vec4(ssbo_BoundingSphere.data[objectIndex].center, 1)).xyz;
	float radius = ssbo_BoundingSphere.data[objectIndex].radius * transform.scale;

    // Frustum culling
	if(!CheckFrustum(center, radius, viewData.frustumRight, viewData.frustumLeft, viewData.frustumTop, viewData.frustumBottom, viewData.zNear, viewData.zFar))
    {
        return;
    }

    uint lodIndex = LODSelection(center, radius, transform.scale, viewData.lodTarget, surfaceBuffer.surfaces[obj.surfaceId].lodOffset, surfaceBuffer.surfaces[obj.surfaceId].lodCount);
    PrepareDrawCmd(lodIndex, objectIndex); 
}