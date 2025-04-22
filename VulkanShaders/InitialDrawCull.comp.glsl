#version 450
#extension GL_GOOGLE_include_directive : require
//#extension GL_EXT_debug_printf : enable
#define COMPUTE_PIPELINE
#include "../VulkanShaderHeaders/ShaderBuffers.glsl"
#include "../VulkanShaderHeaders/CullingShaderData.glsl"

#define CULL  true

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

void main()
{
    // The object index is for the current object's element in the render object
	uint objectIndex = gl_GlobalInvocationID.x;
    if(cullPC.drawCount <= objectIndex)
    {
        return;
    }
    // This shader only processes objects that were visible last frame
    if(visibilityBuffer.visibilities[objectIndex] == 0)
    {
        return;
    }
    RenderObject obj = cullPC.renderObjectBufferAddress.objects[objectIndex];
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
        // Increments the draw count buffer, so that vkCmdDrawIndexedIndirectCount draws the current object
        uint drawID = atomicAdd(indirectDrawCountBuffer.drawCount, 1);
 
        // The LOD index is calculated using a formula, 
        // where the distance to the bounding sphere's surface is taken
        // and the minimum error that would result in acceptable screen-space deviation
        // is computed based on camera parameters
		float distance = max(length(center) - radius, 0);
		float threshold = distance * viewData.lodTarget / transform.scale;
        uint lodIndex = 0;
        uint lodCount = surfaceBuffer.surfaces[obj.surfaceId].lodCount;
		for (uint i = 1; i < lodCount; ++i)
        {
			if (surfaceBuffer.surfaces[obj.surfaceId].lod[i].error < threshold)
            {
				lodIndex = i;
            }
        }

        // Get the selected LOD
        MeshLod currentLod = surfaceBuffer.surfaces[obj.surfaceId].lod[lodIndex];
        // The object index is needed to know which element to access in the per object data buffer
        indirectDrawBuffer.draws[drawID].objectId = objectIndex;
        // Setup the indirect draw commands based on the selected LODs and the vertex offset of the current surface
        indirectDrawBuffer.draws[drawID].indexCount = currentLod.indexCount;
        indirectDrawBuffer.draws[drawID].instanceCount = 1;
        indirectDrawBuffer.draws[drawID].firstIndex = currentLod.firstIndex;
        indirectDrawBuffer.draws[drawID].vertexOffset = surfaceBuffer.surfaces[obj.surfaceId].vertexOffset;
        indirectDrawBuffer.draws[drawID].firstInstance = 0;
    } 
}