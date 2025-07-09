#define TERRAIN

#include "../Headers/vsBuffers.hlsl"
#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/hlslMath.hlsl"

// The main vertex shader function
VSOutput main(uint vertexIndex : SV_VERTEXID)
{
    VSOutput output;

    // Position
    float3 modelPos = ssbo_TerrainVtxPositions[vertexIndex];
    output.position = mul(projectionView, (float4(modelPos, 1.0f)));

    return output;
}