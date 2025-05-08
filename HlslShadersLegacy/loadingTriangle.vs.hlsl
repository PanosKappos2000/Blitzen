struct VSOutput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

static const float3 gVertices[3] = 
{
    float3(0.0f, -0.5f, 0.0f),
    float3(0.5f, 0.5f, 0.0f),
    float3(-0.5f, 0.5f, 0.0f)
};

VSOutput main(uint vertexIndex : SV_VertexID)
{
    VSOutput output;
    output.position = float4(gVertices[vertexIndex], 1.0f);
    output.color = float3(0, 0.8, 0.4);
    
    return output;
}