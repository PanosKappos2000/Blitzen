struct Lod
{
    uint indexCount;
    uint indexOffset;
    uint clusterOffset;
    uint clusterCount;
    float error;
    // Pad to 32 bytes total 
    uint padding0;
    uint padding1;
    uint padding3;
};
StructuredBuffer<Lod> ssbo_LODs : register(t4);

struct BoundingSphere
{
    float3 center;
    float radius;
};
StructuredBuffer<BoundingSphere> ssbo_BoundingSpheres : register(t1);

// The LOD index is calculated using a formula, where the distance to the bounding sphere's surface is taken
// and the minimum error that would result in acceptable screen-space deviation is computed based on camera parameters
uint LODSelection(float3 center, float radius, float scale, float lodTarget, uint lodOffset, uint lodCount)
{
    float distance = max(length(center) - radius, 0);
    float threshold = distance * lodTarget / scale;
    uint lodIndex = 0;
    for (uint i = 1; i < lodCount; ++i)
    {
        if (ssbo_LODs[lodOffset + i].error < threshold)
        {
            lodIndex = i;
        }
    }
    return lodOffset + lodIndex;
}

struct DrawCmd
{
    // Index into render object buffer
    uint objId;

    // Draw command
    uint indexCount;
    uint instCount;
    uint indexOffset;
    int vertOffset;
    uint insOffset;

    uint padding0;
    uint padding1;
};
#ifdef OPAQUE_STATIC_CULL
    RWStructuredBuffer<DrawCmd> ssbo_DrawCmd : register(u0);
    RWBuffer<uint> rwb_DrawCmdCounter : register(u1);
#endif

#ifdef OPAQUE_DYNAMIC_CULL
    RWStructuredBuffer<DrawCmd> ssbo_DrawCmd : register(u2);
    RWBuffer<uint> rwb_DrawCmdCounter : register(u3);
#endif

#ifdef CLUSTER_CULL
    RWStructuredBuffer<DrawCmd> ssbo_DrawCmd : register(u8);
    RWBuffer<uint> rwb_DrawCmdCounter : register(u9);
#endif

#ifdef COLLIDER_CULL
    RWStructuredBuffer<DrawCmd> ssbo_DrawCmd : register(u22);
    RWBuffer<uint> rwb_DrawCmdCount : register(u23);
#endif

#ifndef INSTANCED_CULL
void PrepareDrawCmd(uint lodId, uint objId)
{

    uint cmdId;
    InterlockedAdd(rwb_DrawCmdCounter[0], 1, cmdId);

    // Render object id constant
    ssbo_DrawCmd[cmdId].objId = objId;

    // Vertices
    ssbo_DrawCmd[cmdId].indexCount = ssbo_LODs[lodId].indexCount;
    ssbo_DrawCmd[cmdId].indexOffset = ssbo_LODs[lodId].indexOffset;
    ssbo_DrawCmd[cmdId].vertOffset = 0; // Already added to the index buffer

    // Instances
    ssbo_DrawCmd[cmdId].instCount = 1;
    ssbo_DrawCmd[cmdId].insOffset = 0;
}
#endif