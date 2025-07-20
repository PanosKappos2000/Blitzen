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
    float2 screenCoords = input.position.xy / input.position.w; 
    float dist = length(screenCoords); 

    //if (dist < input.radius)
    //{
    //    return float4(1.0, 0.0, 0.0, 1.0);
    //}
    //else
    //{
    //    discard;
    //}
    
    return float4(0, 0, 0, 0);
}