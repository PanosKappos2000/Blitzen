struct PSOutput
{
    float4 color : SV_TARGET;
};

PSOutput main() : SV_TARGET
{
    PSOutput output;
    output.color = float4(1.0f, 0.0f, 0.0f, 1.0f); 
    return output;
}