struct InstanceDrawCmd
{
    // Draw command
    uint indexCount;
    uint instCount;
    uint indexOffset;
    int vertOffset;
    uint insOffset;

    uint padding0;
    uint padding1;
    uint padding2;
};
RWStructuredBuffer<InstanceDrawCmd> rwssbo_InstDrawCmd : register(u2);