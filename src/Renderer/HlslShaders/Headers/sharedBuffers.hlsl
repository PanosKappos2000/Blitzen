struct Surface
{
    uint materialId;
    uint lodOffset;
    uint lodCount;
    uint padding0;
};
StructuredBuffer<Surface> ssbo_Surfaces : register(t2);

struct Transform
{
    float3 position;
    float scale;
    float4 orientation;
};
RWStructuredBuffer<Transform> ssbo_Transforms : register(u4);

struct Render
{
    uint transformId;
    uint surfaceId;
};
StructuredBuffer<Render> ssbo_Renders : register(t0);

cbuffer ViewData : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projectionView;
    float3 cameraPosition;

    float frustumRight;
    float frustumLeft;
    float frustumTop;
    float frustumBottom;

    float proj0;
    float proj5;

    float zNear;
    float zFar;

    float pyramidWidth;
    float pyramidHeight;

    float lodTarget;
};

#ifdef INSTANCING
    struct InstancedRender
    {
        uint transformId;
        uint drawCommandId;
    };
    StructuredBuffer<InstancedRender> ssbo_Instanced : register(t15);
#endif