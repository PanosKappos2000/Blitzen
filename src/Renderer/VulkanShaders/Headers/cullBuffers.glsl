#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types : require

#define LOD_ENABLED
#define OCCLUSION_ENABLED

struct Lod
{
    // Non cluster path, used to create draw commands
    uint indexCount;
    uint firstIndex;

	// Cluster path, used to create draw commands
    uint clusterOffset;
    uint clusterCount;

    // Used for more accurate LOD selection
    float error;

    // Pad to 32 bytes total
    uint padding0;
    uint padding1;
    uint padding2;
};

layout(set = 0, binding = 4, std430) readonly buffer LodBuffer
{
    Lod levels[];
}lodBuffer;

uint LODSelection(vec3 center, float radius, float scale, float lodTarget, uint lodOffset, uint lodCount)
{
    float distance = max(length(center) - radius, 0);
	float threshold = distance * lodTarget / scale;
    uint lodIndex = 0;
	for (uint i = 1; i < lodCount; ++i)
    {
		if (lodBuffer.levels[lodOffset + i].error < threshold)
        {
			lodIndex = i;
        }
    }
    return lodIndex;
}

struct ClusterGroupData
{
    uint objectId;
    uint lodIndex;
    uint clusterId;

    uint padding0;
};

#ifdef PRE_CLUSTER
layout(buffer_reference, std430) writeonly buffer ClusterDispatchBuffer
{
	ClusterGroupData data[];
};
#else
layout(buffer_reference, std430) readonly buffer ClusterDispatchBuffer
{
	ClusterGroupData data[];
};
#endif

layout(buffer_reference, std430) writeonly buffer ClusterCountBuffer
{
	uint count;
};

// The indirect count buffer holds a single integer that is the draw count for VkCmdDrawIndexedIndirectCount. 
// Will be incremented when necessary by a compute shader
layout(set = 0, binding = 9, std430) writeonly buffer IndirectDrawCount
{
    uint drawCount;
}indirectDrawCountBuffer;

layout(set = 0, binding = 10, std430) buffer VisibilityBuffer
{
    uint visibilities[];
}visibilityBuffer;

#ifdef CLUSTER_CULLING

layout (push_constant) uniform PushConstants
{
    RenderObjectBuffer renderObjectBuffer;
    ClusterDispatchBuffer clusterDispatchBuffer;
    ClusterCountBuffer clusterCountBuffer;
    uint drawCount;
	uint padding0;
}pushConstant;

// Meshlet used in the mesh shader to draw a surface or mesh
struct Cluster
{
    // Bounding sphere for frustum culling
    vec3 center;
    float radius;

    // This is for backface culling
    int8_t coneAxisX;
    int8_t coneAxisY;
    int8_t coneAxisZ;
    int8_t coneCutoff;

    uint dataOffset; // Index into meshlet data
    uint8_t vertexCount;
    uint8_t triangleCount;
    uint8_t padding0;
    uint8_t padding1;
};

// The single buffer that holds all meshlet data in the scene
layout(set = 0, binding = 12, std430) readonly buffer ClusterBuffer
{
    Cluster clusters[];
}clusterBuffer;

#else

layout (push_constant) uniform CullingConstants
{
    RenderObjectBuffer renderObjectBuffer;
    uint drawCount;
	uint padding0;
}pushConstant;

#endif