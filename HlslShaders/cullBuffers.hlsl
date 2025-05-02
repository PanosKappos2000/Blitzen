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
    uint padding0;  
    uint padding1; 
    uint padding2;
};
StructuredBuffer<Lod> lodBuffer : register (t7);