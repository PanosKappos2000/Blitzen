#include "../../Resources/blitShaderShared.h"
#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/colliders.hlsl"

struct VSOutput
{
    float4 position : SV_POSITION; 
    float radius : RADIUS; 
};

cbuffer cb_objID : register(b12)
{
    uint objID;
};

VSOutput main()
{
    VSOutput output;
    output.position = float4(0.f, 0.f, 0.f, 0.f);
    output.radius = 0.f;
    return output;
}