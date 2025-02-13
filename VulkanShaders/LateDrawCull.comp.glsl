#version 450

#extension GL_GOOGLE_include_directive : require

#define COMPUTE_PIPELINE
#define LOD_ENABLED
#define OCCLUSION_ENABLED

#include "../VulkanShaderHeaders/ShaderBuffers.glsl"
#include "../VulkanShaderHeaders/CullingShaderData.glsl"

#define CULL  true

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout (set = 0, binding = 3) uniform sampler2D depthPyramid;

layout (push_constant) uniform constants
{
    uint32_t postPass;
};

void main()
{
    uint objectIndex = gl_GlobalInvocationID.x;

    // This a guard so that the compute shader does not go over the draw count
    if(cullingData.drawCount <= objectIndex)
        return;

    // Access the object's data
    RenderObject object = objectBuffer.objects[objectIndex];
    Transform transform = transformBuffer.instances[object.meshInstanceId];
    Surface surface = surfaceBuffer.surfaces[object.surfaceId];

    // If the late culling shader does not match the pass of the current surface it exits
    if(surface.postPass != postPass)
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

    // Later draw culling also does occlusion culling on objects that passed the frustum culling test above
    #ifdef OCCLUSION_ENABLED
    if (visible)
	{
		vec4 aabb;
		if (projectSphere(center, radius, cullingData.zNear, cullingData.proj0, cullingData.proj5, aabb))
		{
			float width = (aabb.z - aabb.x) * cullingData.pyramidWidth;
			float height = (aabb.w - aabb.y) * cullingData.pyramidHeight;

            // Find the mip map level that will match the screen size of the sphere
			float level = floor(log2(max(width, height)));

			float depth = textureLod(depthPyramid, (aabb.xy + aabb.zw) * 0.5, level).x;

			float depthSphere = cullingData.zNear / (center.z - radius);

			visible = visible && depthSphere > depth;
		}
	}
    #endif

    // The late culling shader creates draw commands for the objects that passed late culling and were not tagged as visible last frame
    // It handles transparent objects a little bit differently as this is the only shader that will cull them
    if(visible && (visibilityBuffer.visibilities[objectIndex] == 0 || postPass != 0))
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
        #ifdef LOD_ENABLED
		float distance = max(length(center) - radius, 0);
		float threshold = distance * cullingData.lodTarget / transform.scale;
		for (uint i = 1; i < surface.lodCount; ++i)
			if (surface.lod[i].error < threshold)
				lodIndex = i;
		#endif

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
        /*indirectTaskBuffer.tasks[drawIndex].taskId = currentLod.firstMeshlet;
        indirectTaskBuffer.tasks[drawIndex].groupCountX = (currentLod.meshletCount + 31) / 32;
        indirectTaskBuffer.tasks[drawIndex].groupCountY = 1;
        indirectTaskBuffer.tasks[drawIndex].groupCountZ = 1;*/
    }

    // Any object that passed both occlusion and frustum culling, will have its visibility set to 1 for next frame
    // That means that the early culling shader will perform furstum culling on them
    visibilityBuffer.visibilities[objectIndex] = visible ? 1 : 0;
}