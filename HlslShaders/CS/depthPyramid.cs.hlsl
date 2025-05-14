Texture2D<float4> inImage : register(t0);
RWTexture2D<float4> outImage : register(u0);

cbuffer PyramidMip : register(b0)
{
    uint width;
    uint height;
    uint level;
};

// Define the workgroup size
[numthreads(32, 32, 1)] 
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 texSize = uint2(width, height); 

    // Fetch the depth value from the current mip level
    //float depth = inImage.Load(dispatchThreadID.xy).r;
    float depth = inImage.Load(int3(dispatchThreadID.xy + float2(0.5, 0.5), level)).r;

    // Use some form of reduction, such as averaging or min/max, depending on your needs.
    // This example does a simple min reduction, comparing with the surrounding pixels:
    if (dispatchThreadID.x < texSize.x - 1 && dispatchThreadID.y < texSize.y - 1)
    {
        float depthRight = inImage.Load(int3(dispatchThreadID.xy + uint2(1, 0), level)).r;
        float depthDown = inImage.Load(int3(dispatchThreadID.xy + uint2(0, 1), level)).r;
        
        depth = min(depth, min(depthRight, depthDown));
    }

    // Store the result in the output texture at the same location
    outImage[dispatchThreadID.xy] = depth;
}