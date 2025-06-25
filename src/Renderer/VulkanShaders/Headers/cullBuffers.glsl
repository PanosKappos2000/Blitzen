#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types : require

#define LOD_ENABLED
#define OCCLUSION_ENABLED

struct Lod
{
    uint indexCount;
    uint firstIndex;

    uint clusterOffset;
    uint clusterCount;

    float error;

    // Pad to 32 bytes total
    uint padding0;
    uint padding1;
    uint padding2;
};

layout(set = 0, binding = 4, std430) readonly buffer SSBO_LOD
{
    Lod data[];
}ssbo_LODs;

uint LODSelection(vec3 center, float radius, float scale, float lodTarget, uint lodOffset, uint lodCount)
{
    float distance = max(length(center) - radius, 0);
	float threshold = distance * lodTarget / scale;
    uint lodIndex = 0;
	for (uint i = 1; i < lodCount; ++i)
    {
		if (ssbo_LODs.data[lodOffset + i].error < threshold)
        {
			lodIndex = i;
        }
    }
    return lodIndex + lodOffset;
}

layout(set = 0, binding = 7, std430) writeonly buffer RWSSBO_DRAW_CMD
{
        IndirectDraw data[];
}rwssbo_DrawCmd;

layout(set = 0, binding = 9, std430) writeonly buffer RWSSBO_DRAW_CMD_COUNT
{
    uint data;
}rwssbo_DrawCount;

void PrepareDrawCmd(uint lodIndex, uint objectIndex)
{
    // Increments draw count
    uint drawID = atomicAdd(rwssbo_DrawCount.data, 1);

    // object id
    rwssbo_DrawCmd.data[drawID].objectId = objectIndex;

    // vertices
    rwssbo_DrawCmd.data[drawID].indexCount = ssbo_LODs.data[lodIndex].indexCount;
    rwssbo_DrawCmd.data[drawID].firstIndex = ssbo_LODs.data[lodIndex].firstIndex;
    rwssbo_DrawCmd.data[drawID].vertexOffset = 0;
        
    // instances
    rwssbo_DrawCmd.data[drawID].instanceCount = 1;
    rwssbo_DrawCmd.data[drawID].firstInstance = 0;
}

struct BoundingSphere
{
    vec3 center;
    float radius;
};
layout(set = 0, binding = 15, std430) readonly buffer SSBO_BOUNDING_SPHERES
{
    BoundingSphere data[];
}ssbo_BoundingSphere;

#ifdef DOUBLE_PASS

layout(set = 0, binding = 10, std430) buffer RWSSBO_DRAW_VIS
{
    uint data[];
}rwssbo_DrawVis;

#endif

#ifdef OCCLUSION_CULLING

layout (set = 0, binding = 3) uniform sampler2D depthPyramid;

#endif

layout (push_constant) uniform CullingConstants
{
    uint drawCount;
}pushConstant;