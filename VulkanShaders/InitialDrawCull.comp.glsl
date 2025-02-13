#version 450

#extension GL_GOOGLE_include_directive : require

#define COMPUTE_PIPELINE

#include "../VulkanShaderHeaders/ShaderBuffers.glsl"
#include "../VulkanShaderHeaders/CullingShaderData.glsl"

#define CULL  true

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

void main()
{
    // The object index is for the current object's element in the render object
	uint objectIndex = gl_GlobalInvocationID.x;

    // This is a guard so that the culling shader does not go over the draw count
    if(cullingData.drawCount <= objectIndex)
        return;

    // This culling shader also returns if the current object was not visible last frame
    if(visibilityBuffer.visibilities[objectIndex] == 0)
        return;

    // Gets the current object using the global invocation ID. It also retrieves the surface that the objects points to and the transform data
    RenderObject currentObject = objectBuffer.objects[objectIndex];
    Transform transform = transformBuffer.instances[currentObject.meshInstanceId];
    Surface surface = surfaceBuffer.surfaces[currentObject.surfaceId];

    // The initial culling pass does not touch transparent objects
    if(surface.postPass == 1)
        return;

    // Promotes the bounding sphere's center to model and the view coordinates (frustum culling will be done on view space)
    vec3 center = RotateQuat(surface.center, transform.orientation) * transform.scale + transform.pos;
    center = (shaderData.view * vec4(center, 1)).xyz;

    // The bounding sphere's radius only needs to be multiplied by the object's scale
	float radius = surface.radius * transform.scale;

    // Check that the bounding sphere is inside the view frustum(frustum culling)
	bool visible = true;
    // the left/top/right/bottom plane culling utilizes frustum symmetry to cull against two planes at the same time
    // Formula taken from Arseny Kapoulkine's Niagara renderer https://github.com/zeux/niagara
    // It is also referenced in VKguide's GPU driven rendering articles https://vkguide.dev/docs/gpudriven/compute_culling/
    visible = visible && center.z * cullingData.frustumLeft - abs(center.x) * cullingData.frustumRight > -radius;
	visible = visible && center.z * cullingData.frustumBottom - abs(center.y) * cullingData.frustumTop > -radius;
	// the near/far plane culling uses camera space Z directly
	visible = visible && center.z + radius > cullingData.zNear && center.z - radius < cullingData.zFar;
	
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
        if (cullingData.lodEnabled == 1)
		{
			float distance = max(length(center) - radius, 0);
			float threshold = distance * cullingData.lodTarget / transform.scale;
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

        // Indirect task commands
        /*bufferAddrs.indirectTaskBuffer.tasks[drawIndex].taskId = currentLod.firstMeshlet;
        bufferAddrs.indirectTaskBuffer.tasks[drawIndex].groupCountX = (currentLod.meshletCount + 31) / 32;
        bufferAddrs.indirectTaskBuffer.tasks[drawIndex].groupCountY = 1;
        bufferAddrs.indirectTaskBuffer.tasks[drawIndex].groupCountZ = 1;*/
    } 
}