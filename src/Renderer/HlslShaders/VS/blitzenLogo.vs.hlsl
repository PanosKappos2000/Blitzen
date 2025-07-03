struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

static const float3 gVertices[4] =
{
    float3(-1.0f, -1.f, 0.0f),
    float3(-1.0f, 1.0f, 0.0f),
    float3(1.0f, 1.0f, 0.0f),
    float3(1.0f, -1.0f, 0.0f)
};

static const float2 gUVs[4] =
{
    float2(0.0f, 0.0f), // Bottom-left UV
    float2(0.0f, 1.0f), // Top-left UV
    float2(1.0f, 1.0f), // Top-right UV
    float2(1.0f, 0.0f) // Bottom-right UV
};

VSOutput main(uint vertexIndex : SV_VertexID)
{
    VSOutput output;
    output.position = float4(gVertices[vertexIndex], 1.0f);
    output.uv = gUVs[vertexIndex];
    
    return output;
}