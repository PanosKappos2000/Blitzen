#version 450

#extension GL_EXT_mesh_shader: require
#extension GL_GOOGLE_include_directive: require
#extension GL_ARB_shader_draw_parameters : require

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

#include "../VulkanShaderHeaders/ShaderBuffers.glsl"

taskPayloadSharedEXT MeshTaskPayload payload;

bool coneCull(vec4 cone, vec3 view)
{
	return dot(cone.xyz, view) > cone.w;
}

void main()
{
    uint threadIndex = gl_LocalInvocationID.x;
	uint meshletGroupIndex = gl_WorkGroupID.x;

	uint drawId = indirectDrawBuffer.draws[gl_DrawIDARB].objectId;
    RenderObject currentObject = objectBuffer.objects[drawId];
	Transform meshDraw = transformBuffer.instances[currentObject.meshInstanceId];
    Surface currentSurface = surfaceBuffer.surfaces[currentObject.surfaceId];

    uint meshletIndex = meshletGroupIndex * 32 + threadIndex + indirectTaskBuffer.tasks[gl_DrawIDARB].taskId;

    payload.drawId = drawId;
    payload.meshletIndices[threadIndex] = meshletIndex;
    EmitMeshTasksEXT(32, 1, 1);
}