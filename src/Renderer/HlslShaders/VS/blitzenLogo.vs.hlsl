struct VSOutput
{
    float4 position : SV_POSITION;
};

static const float3 gVertices[4] =
{
    float3(-1.0f, -1.f, 0.0f),
    float3(-1.0f, 1.0f, 0.0f),
    float3(1.0f, 1.0f, 0.0f),
    float3(1.0f, -1.0f, 0.0f)
};

VSOutput main(uint vertexIndex : SV_VertexID)
{
    VSOutput output;
    output.position = float4(gVertices[vertexIndex], 1.0f);
    
    return output;
}