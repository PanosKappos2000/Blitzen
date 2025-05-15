Texture2D<float4> depthTarget : register(t0);
RWTexture2D<float4> depthPyramid : register(u0);

cbuffer PyramidMip : register(b0)
{
    float2 imageSize;
    uint level;
};

// Define the workgroup size
[numthreads(32, 32, 1)] 
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    float depth = 0;
    int2 texCoords = round((float2(dispatchThreadID.xy) + float2(0.5f, 0.5f)) / imageSize);
    // Starts with depth target
    if(level == 0)
    {
        depth = depthTarget.Load(int3(texCoords, level)).r;
    }
    // Loads previous mip
    else
    {
        
        depth = depthPyramid.Load(int3(texCoords, level - 1)).r;
    }

    // Use some form of reduction, such as averaging or min/max, depending on your needs.
    // This example does a simple min reduction, comparing with the surrounding pixels:
    if (dispatchThreadID.x < imageSize.x - 1 && dispatchThreadID.y < imageSize.y - 1)
    {
        float depthRight = depthPyramid.Load(int3(texCoords + int2(1, 0), level)).r;
        float depthDown = depthPyramid.Load(int3(texCoords + int2(0, 1), level)).r;
        
        depth = min(depth, min(depthRight, depthDown));
    }

    depthPyramid[texCoords] = depth;
}