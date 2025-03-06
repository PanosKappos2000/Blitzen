#version 450

#extension GL_GOOGLE_include_directive : require

#define COMPUTE_PIPELINE

#include "../VulkanShaderHeaders/ShaderBuffers.glsl"
#include "../VulkanShaderHeaders/CullingShaderData.glsl"

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

void main()
{
    // The object index is for the current object's element in the render object
	uint objectIndex = gl_GlobalInvocationID.x;

    // Gets the current object using the global invocation ID. It also retrieves the surface that the objects points to and the transform data
    RenderObject currentObject = onpcReflectiveObjectBuffer.objects[objectIndex];
    Transform transform = transformBuffer.instances[currentObject.meshInstanceId];
    Surface surface = surfaceBuffer.surfaces[currentObject.surfaceId];

    /*vec3 center;
    vec3 radius;
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
        // With each element that is added to the draw list, increment the count buffer
        uint drawIndex = atomicAdd(indirectCountBuffer.drawCount, 1);

        // The lod index is declared here. if LODs are not enabled the most detailed version of an object will be used by default
        uint lodIndex = 0;
        /*  
            The LOD index is calculated using a formula where the distance to bounding sphere
            surface is taken and the minimum error that would result in acceptable
            screen-space deviation is computed based on camera parameters
        */
        /*if (cullPC.lodEnabled == 1)
		{
			float distance = max(length(center) - radius, 0);
			float threshold = distance * viewData.lodTarget / transform.scale;
			for (uint i = 1; i < surface.lodCount; ++i)
				if (surface.lod[i].error < threshold)
					lodIndex = i;
		}

        // Get the selected LOD
        MeshLod currentLod = surface.lod[lodIndex];
        // The object index is needed to know which element to access in the per object data buffer
        indirectDrawBuffer.draws[drawIndex].objectId = objectIndex;
        // Setup the indirect draw commands based on the selected LODs and the vertex offset of the current surface
        indirectDrawBuffer.draws[drawIndex].indexCount = currentLod.indexCount;
        indirectDrawBuffer.draws[drawIndex].instanceCount = 1;
        indirectDrawBuffer.draws[drawIndex].firstIndex = currentLod.firstIndex;
        indirectDrawBuffer.draws[drawIndex].vertexOffset = surface.vertexOffset;
        indirectDrawBuffer.draws[drawIndex].firstInstance = 0;
    }*/
}