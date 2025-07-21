struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    uint materialId : TEXCOORD1;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
};

float4 main(PSInput input) : SV_Target
{
    return float4(1.0f, 0, 0, 0.3f);
}