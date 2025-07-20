struct PSInput
{
    float4 position : SV_POSITION; 
    float radius : RADIUS; 
};

float4 main(PSInput input) : SV_Target
{
    float2 screenCoords = input.position.xy / input.position.w; 
    float dist = length(screenCoords); 

    if (dist < input.radius)
    {
        return float4(1.0, 0.0, 0.0, 1.0);
    }
    else
    {
        discard;
    }
    
    return float4(0, 0, 0, 0);
}