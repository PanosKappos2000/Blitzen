#define OPAQUE_DYNAMIC_CULL

/*
    THIS ONE IS JUST A DRAFT
*/
#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/dynamicCull.hlsl"
#include "../Headers/occlusionCull.hlsl"
#include "../Headers/cullOut.hlsl"
#include "../Headers/hlslMath.hlsl"
#include "../Headers/cpuShared.h"

cbuffer cb_MoveCount : register(b20)
{
    uint moveCount;
};

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 dispatchGroupID : SV_GroupID)
{
    //uint objId = dispatchThreadID.x;
    //
    //// Early return if it's out of bounds
    //if (objId >= moveCount)
    //{
    //    return;
    //}
    //
    //Movement movement = ssbo_Movements[objId];
    //Transform transform = ssbo_Transforms[movement.padding0];
    //
    //ssbo_Transforms[movement.padding0].position = movement.velocity;
    //
    //if(movement.padding1 == 1)
    //{
    //    float4 orientation = float4(0.f, 0.f, 0.f, 1.f);
    //    if (true /*movement.rotation.x != 0*/)
    //    {
    //        float4 orientationYaw = NormalizedQuatFromAngleAxis(float3(0.f, -1.f, 0.f), movement.rotation.x);
    //        orientation = MultiplyQuat(orientation, orientationYaw);
    //    }
    //    if (true /*movement.rotation.y != 0*/)
    //    {
    //        float4 orientationPitch = NormalizedQuatFromAngleAxis(float3(1.f, 0.f, 0.f), movement.rotation.y);
    //        orientation = MultiplyQuat(orientation, orientationPitch);
    //    }
    //    if (true)
    //    {
    //    
    //    }
    //    ssbo_Transforms[movement.padding0].orientation = orientation;
    //}
}