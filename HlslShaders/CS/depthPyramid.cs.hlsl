Texture2D<float4> depthTarget : register(t0);
RWTexture2D<float4> depthPyramid : register(u0);

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
    
    float depth = 0;
    // Starts with depth target
    if(level == 0)
    {
        depth = depthTarget.Load(uint3(dispatchThreadID.xy + uint2(0.5, 0.5), level)).r;
    }
    // Loads previous mip
    else
    {
        depth = depthPyramid.Load(uint3(dispatchThreadID.xy + uint2(0.5, 0.5), level - 1)).r;
    }

    // Use some form of reduction, such as averaging or min/max, depending on your needs.
    // This example does a simple min reduction, comparing with the surrounding pixels:
    if (dispatchThreadID.x < texSize.x - 1 && dispatchThreadID.y < texSize.y - 1)
    {
        float depthRight = depthPyramid.Load(uint3(dispatchThreadID.xy + uint2(1, 0), level)).r;
        float depthDown = depthPyramid.Load(uint3(dispatchThreadID.xy + uint2(0, 1), level)).r;
        
        depth = min(depth, min(depthRight, depthDown));
    }

    depthPyramid[dispatchThreadID.xy] = depth;
}