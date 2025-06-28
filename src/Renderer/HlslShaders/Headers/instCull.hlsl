struct InstDrawCmd
{
    uint instanceOffset;
    uint resourceId;
    uint indexCount;
    uint instCount;
    uint indexOffset;
    int vertOffset;
    uint insOffset;

    uint padding0;
};
RWStructuredBuffer<InstDrawCmd> rwssbo_DrawCmd : register(u11);

cbuffer ObjCountConstant : register(b9)
{
    uint workCount;
};
