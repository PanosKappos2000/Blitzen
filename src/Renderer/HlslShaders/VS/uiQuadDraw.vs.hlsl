struct DSQuad
{
    float2 position;
    float2 scale;
    float4 color;
};
StructuredBuffer<DSQuad> PanelQuads : register(t20);

struct VSOutput
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

static const float2 QuadVertices[6] =
{
    float2(-1.f, -1.f),
    float2(-1.f, 1.f),
    float2(1.f, 1.f),
    float2(1.f, -1.f),
    float2(-1.f, -1.f),
    float2(1.f, 1.f)
};

cbuffer cbProjection : register(b20)
{
    float4x4 projection;
};

VSOutput main(uint vertexIndex : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VSOutput output;
    
    float2 worldPos = QuadVertices[vertexIndex] * PanelQuads[instanceID].scale + PanelQuads[instanceID].position;
    output.position = mul(projection, float4(worldPos.x, worldPos.y, 1.f, 1.f));
    
    output.color = PanelQuads[vertexIndex].color;
    
    return output;
}