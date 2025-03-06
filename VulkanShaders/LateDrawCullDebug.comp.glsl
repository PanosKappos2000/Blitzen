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
    if(cullPC.drawCount <= objectIndex)
        return;

    // Access the object's data
    RenderObject object = objectBuffer.objects[objectIndex];
    Transform transform = transformBuffer.instances[object.meshInstanceId];
    Surface surface = surfaceBuffer.surfaces[object.surfaceId];

    // If the late culling shader does not match the pass of the current surface, it exits
    if(surface.postPass != cullPC.postPass)
        return;
    
    vec3 center;
	float radius;
	bool visible = IsObjectInsideViewFrustum(
        center, radius, surface.center, surface.radius, 
        transform.scale, transform.pos, transform.orientation, 
        viewData.view, 
        viewData.frustumRight, viewData.frustumLeft, 
        viewData.frustumTop, viewData.frustumBottom,
        viewData.zNear, viewData.zFar);

    // Later draw culling also does occlusion culling on objects that passed the frustum culling test above
    if (visible && uint(cullPC.occlusionEnabled) == 1)
	{
		vec4 aabb;
		if (projectSphere(center, radius, viewData.zNear, viewData.proj0, viewData.proj5, aabb))
		{
			float width = (aabb.z - aabb.x) * viewData.pyramidWidth;
			float height = (aabb.w - aabb.y) * viewData.pyramidHeight;

            // Find the mip map level that will match the screen size of the sphere
			float level = floor(log2(max(width, height)));

			float depth = textureLod(depthPyramid, (aabb.xy + aabb.zw) * 0.5, level).x;

			float depthSphere = viewData.zNear / (center.z - radius);

			visible = visible && depthSphere > depth;
		}
	}

    // The late culling shader creates draw commands for the objects that passed late culling and were not tagged as visible last frame
    // It handles transparent objects a little bit differently as this is the only shader that will cull them
    if(visible && (visibilityBuffer.visibilities[objectIndex] == 0 || cullPC.postPass != 0))
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
        if (cullPC.lodEnabled == 1)
		{
			float distance = max(length(center) - radius, 0);
			float threshold = distance * viewData.lodTarget / transform.scale;
			for (uint i = 1; i < surface.lodCount; ++i)
				if (surface.lod[i].error < threshold)
					lodIndex = i;
		}

        // Gets the selected LOD
        MeshLod currentLod = surface.lod[lodIndex];
        // The object index is needed to know which element to access in the per object data buffer
        indirectDrawBuffer.draws[drawIndex].objectId = objectIndex;
        // Sets up the indirect draw commands based on the selected LODs and the vertex offset of the current surface
        indirectDrawBuffer.draws[drawIndex].indexCount = currentLod.indexCount;
        indirectDrawBuffer.draws[drawIndex].instanceCount = 1;
        indirectDrawBuffer.draws[drawIndex].firstIndex = currentLod.firstIndex;
        indirectDrawBuffer.draws[drawIndex].vertexOffset = surface.vertexOffset;
        indirectDrawBuffer.draws[drawIndex].firstInstance = 0;
    }

    // Any object that passed both occlusion and frustum culling, will have its visibility set to 1 for next frame
    // That means that the early culling shader will perform furstum culling on them
    visibilityBuffer.visibilities[objectIndex] = visible ? 1 : 0;
}