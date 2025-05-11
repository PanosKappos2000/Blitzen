struct ComputeCmd
{
    uint groupX;
    uint groupY;
    uint groupZ;
};

RWBuffer<uint> drawCountBuffer : register(u1);

struct Lod 
{
    // Non cluster path, used to create draw commands
    uint indexCount;
    uint indexOffset;

    // Cluster path, used to create draw commands
    uint clusterOffset;
    uint clusterCount;

    // Used for more accurate LOD selection
    float error;

    // Pad to 32 bytes total 
    uint instanceId;  
    uint instanceCount;

    uint padding0;
};
#ifdef DRAW_INSTANCING
    RWStructuredBuffer<Lod> lodBuffer : register (u2);
#else
    StructuredBuffer<Lod> lodBuffer : register (t7);
#endif

#ifdef DRAW_INSTANCING

RWStructuredBuffer<uint> instBuffer : register(u3);// TODO: This should be in shared (needs to be accessed by vertex)

#endif

// The LOD index is calculated using a formula, 
// where the distance to the bounding sphere's surface is taken
// and the minimum error that would result in acceptable screen-space deviation
// is computed based on camera parameters
uint LODSelection(float3 center, float radius, float scale, float lodTarget, uint lodOffset, uint lodCount)
{
    float distance = max(length(center) - radius, 0);
	float threshold = distance * lodTarget / scale;
    uint lodIndex = 0;
	for (uint i = 1; i < lodCount; ++i)
    {
		if (lodBuffer[lodOffset + i].error < threshold)
        {
			lodIndex = i;
        }
    }
    return lodOffset + lodIndex;
}