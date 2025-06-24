struct VSOutput
{
    float4 position : SV_POSITION; 
    float radius : RADIUS; 
};

struct BoundingSphere
{
    float3 center;
    float radius;
};
StructuredBuffer<BoundingSphere> ssbo_BoundingSpheres : register(t0);

cbuffer ObjId : register(b1)
{
    uint objId;
};

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

VSOutput main()
{
    VSOutput output;
    
    BoundingSphere sphere = ssbo_BoundingSpheres[objId];
    output.position = mul(float4(sphere.center, 1.0f), projectionView);
    output.radius = sphere.radius;
    return output;
}