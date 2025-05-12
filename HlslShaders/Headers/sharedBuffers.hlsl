struct Surface
{
    // Bounding sphere
    float3 center;     
    float radius;            

    uint materialId;

    // Index to lod buffer
    // Each primitive can be drawn in multiple ways, depending on its LODs
    // The LOD is selected in compute shaders
    uint lodOffset;
    uint lodCount;

    uint vertexOffset; // Not used in the shaders but can hold the offset when loading
};
StructuredBuffer<Surface> surfaceBuffer : register(t2);

struct Transform
{
    float3 position;
    float scale;
    float4 orientation;
};
StructuredBuffer<Transform> transformBuffer : register(t3);

struct Render
{
    uint transformId;
    uint surfaceId;
};
StructuredBuffer<Render> renderBuffer : register(t4);

#ifdef DRAW_INSTANCING

RWStructuredBuffer<uint> instBuffer : register(u3);

#endif

cbuffer ViewData : register(b0)  
{
    float4x4 viewMatrix;
    float4x4 projectionView;
    float3 position;

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