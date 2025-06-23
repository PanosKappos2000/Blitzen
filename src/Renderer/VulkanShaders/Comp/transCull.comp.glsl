#version 450
#extension GL_GOOGLE_include_directive : require
//#extension GL_EXT_debug_printf : enable

#define COMPUTE_PIPELINE
#include "../Headers/sharedBuffers.glsl"
#include "../Headers/cullBuffers.glsl"
#include "../Headers/math.glsl"
#include "../Headers/bufferOffsets.glsl"
#define CULL  true

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout (set = 0, binding = 3) uniform sampler2D depthPyramid;

void main()
{
    uint objectIndex = gl_GlobalInvocationID.x + BLIT_TRANSPARENT_RENDER_OFFSET;

    if(pushConstant.drawCount + BLIT_TRANSPARENT_RENDER_OFFSET <= objectIndex)
    {
        return;
    }

    RenderObject obj = ssbo_render.data[objectIndex];
    Transform transform = transformBuffer.instances[obj.meshInstanceId];
    
    // Frustum culling
    vec3 center;
	float radius;
	if(!CheckFrustum(center, radius, ssbo_BoundingSphere.data[objectIndex].center, ssbo_BoundingSphere.data[objectIndex].radius, transform.scale, transform.pos, transform.orientation, viewData.view, 
        viewData.frustumRight, viewData.frustumLeft, viewData.frustumTop, viewData.frustumBottom, viewData.zNear, viewData.zFar))
    {
        return;
    }

    vec4 aabb;
    if (ProjectSphere(center, radius, viewData.zNear, viewData.proj0, viewData.proj5, aabb))
    {
		if(!CheckOcclusion(aabb, depthPyramid, viewData.pyramidWidth, viewData.pyramidHeight, center, radius, viewData.zNear))
        {
            return;
        }
	}

    uint lodIndex = LODSelection(center, radius, transform.scale, viewData.lodTarget, surfaceBuffer.surfaces[obj.surfaceId].lodOffset, surfaceBuffer.surfaces[obj.surfaceId].lodCount);
    PrepareDrawCmd(lodIndex, objectIndex);    
}