struct PSOutput
{
    float4 color : SV_TARGET;
};

#include "../../Resources/blitShaderShared.h"
#include "../Headers/psBuffers.hlsl"
#include "../Headers/hlslMath.hlsl"

PSOutput main(VSOutput input)
{
    PSOutput output;
    output.color = float4(1.f, 0.f, 0.f, 1.f);
    return output;
}