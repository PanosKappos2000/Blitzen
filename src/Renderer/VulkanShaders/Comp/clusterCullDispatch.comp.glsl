#version 450
#extension GL_GOOGLE_include_directive : require
//#extension GL_EXT_debug_printf : enable

#define OCCLUSION_CULLING
#define COMPUTE_PIPELINE
#define CLUSTER_CULLING
#define PRE_CLUSTER
#include "../Headers/sharedBuffers.glsl"
#include "../Headers/cullBuffers.glsl"
#include "../Headers/clusterBuffers.glsl"
#include "../Headers/math.glsl"
#include "../Headers/bufferOffsets.glsl"

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

void main()
{
    uint objectIndex = gl_GlobalInvocationID.x + BLIT_OPAQUE_STATIC_RENDER_OFFSET;
    if (pushConstant.drawCount + BLIT_OPAQUE_STATIC_RENDER_OFFSET <= objectIndex)
    {
        return;
    }
    
    RenderObject obj = ssbo_render.data[objectIndex];
    Transform transform = transformBuffer.instances[obj.meshInstanceId];

    // Bounding sphere to world coordinates
    vec3 center = (viewData.view * vec4(ssbo_BoundingSphere.data[objectIndex].center, 1)).xyz;
	float radius = ssbo_BoundingSphere.data[objectIndex].radius;

    // Frustum culling
	if(!CheckFrustum(center, radius, viewData.frustumRight, viewData.frustumLeft, viewData.frustumTop, viewData.frustumBottom, viewData.zNear, viewData.zFar))
    {
        return;
    }

    // Occlusion culling
    vec4 aabb;
    if (ProjectSphere(center, radius, viewData.zNear, viewData.proj0, viewData.proj5, aabb))
    {
		if(!CheckOcclusion(aabb, depthPyramid, viewData.pyramidWidth, viewData.pyramidHeight, center, radius, viewData.zNear))
        {
            return;
        }
	}

    uint lodIndex = LODSelection(center, radius, transform.scale, viewData.lodTarget, surfaceBuffer.surfaces[obj.surfaceId].lodOffset, surfaceBuffer.surfaces[obj.surfaceId].lodCount);

    // TODO: Replace with group style used in HLSL
    uint clusterCount = ssbo_LODs.data[lodIndex].clusterCount;
    uint clusterOffset = ssbo_LODs.data[lodIndex].clusterOffset;
    uint dispatchIndex = atomicAdd(rwssbo_ClusterCount.data, clusterCount);
    for(uint i = 0; i < clusterCount; ++i)
    {
        rwssbo_ClusterGroup.data[i + dispatchIndex].clusterId = clusterOffset + i;
        rwssbo_ClusterGroup.data[i + dispatchIndex].lodIndex = lodIndex;
        rwssbo_ClusterGroup.data[i + dispatchIndex].objectId = objectIndex;
    }
}