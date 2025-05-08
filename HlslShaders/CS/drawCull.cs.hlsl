#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/hlslMath.hlsl"

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
        uint lodId = LODSelection(center, radius, transform.scale, lodTarget, surface.lodOffset, surface.lodCount);

        // Command count
        uint cmdId;
        InterlockedAdd(drawCountBuffer[0], 1, cmdId);

        // Render object id constant
        drawCmdBuffer[cmdId].objId = objId;

        // Draw commands
        drawCmdBuffer[cmdId].indexCount = lodBuffer[lodId].indexCount;
        drawCmdBuffer[cmdId].instCount = 1;
        drawCmdBuffer[cmdId].indexOffset = lodBuffer[lodId].indexOffset;
        drawCmdBuffer[cmdId].vertOffset = 0;
        drawCmdBuffer[cmdId].insOffset = 0;
    }
}