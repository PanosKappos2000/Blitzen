#include "../../Resources/blitShaderShared.h"
#include "../Headers/sharedBuffers.hlsl"

struct VSOutput
{
    float4 position : SV_POSITION;
};

StructuredBuffer<float3> vertices : register(t19);

VSOutput main(uint vertexIndex : SV_VERTEXID)
{
    VSOutput output;
    
    output.position = mul(projectionView, float4(vertices[vertexIndex], 1.0f));
    
    return output;
}