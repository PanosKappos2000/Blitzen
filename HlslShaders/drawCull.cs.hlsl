#include "HlslShaders/sharedBuffers.hlsl"
#include "HlslShaders/cullBuffers.hlsl"
#include "HlslShaders/hlslMath.hlsl"

RWBuffer<uint> drawCountBuffer : register(u1);

cbuffer ObjCountConstant: register (b2)
{
    uint objCount;
};

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 dispatchGroupID : SV_GroupID)
{
    uint objId = dispatchThreadID.x;
    // Early return if it's out of bounds
    if(objId >= objCount)
    {
        return;
    }
    Render obj = renderBuffer[objId];
    Surface surface = surfaceBuffer[obj.surfaceId];
    Transform transform = transformBuffer[obj.transformId];

    // Promotes the bounding sphere's center to model and the view coordinates (frustum culling will be done on view space)
    float3 center = RotateQuat(surface.center, transform.orientation) * transform.scale + transform.position;
    center = mul(viewMatrix, float4(center, 1)).xyz;
	float radius = surface.radius * transform.scale;
    // Frustum culling
    bool visible = FrustumCheck(center, radius, frustumRight, frustumLeft, frustumTop, frustumBottom, zNear, zFar);

    if(visible)
    {
        uint cmdId;
        InterlockedAdd(drawCountBuffer[0], 1, cmdId);
        drawCmdBuffer[cmdId].objId = objId;
        drawCmdBuffer[cmdId].indexCount = lodBuffer[0 + surface.lodOffset].indexCount;
        drawCmdBuffer[cmdId].instCount = 1;
        drawCmdBuffer[cmdId].indexOffset = lodBuffer[0 + surface.lodOffset].indexOffset;
        drawCmdBuffer[cmdId].vertOffset = 0;
        drawCmdBuffer[cmdId].insOffset = 0;
    }
}