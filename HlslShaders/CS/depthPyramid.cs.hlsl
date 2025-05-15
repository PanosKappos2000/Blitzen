Texture2D<float4> depthIn : register(t0);
RWTexture2D<float4> depthOut : register(u0);

cbuffer PyramidMip : register(b0)
{
    float2 imageSize;
    uint mipLevel;
};

// Define the workgroup size
[numthreads(32, 32, 1)] 
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    int2 texCoords = int2(round((float2(dispatchThreadID.xy) + float2(0.5f, 0.5f)) / imageSize));
    float depth = depthIn.Load(int3(texCoords, mipLevel)).r;

    // Use some form of reduction, such as averaging or min/max, depending on your needs.
    // This example does a simple min reduction, comparing with the surrounding pixels:
    if (dispatchThreadID.x < imageSize.x - 1 && dispatchThreadID.y < imageSize.y - 1)
    {
        float depthRight = depthIn.Load(int3(texCoords.x + 1, texCoords.y, mipLevel)).r;
        float depthDown = depthIn.Load(int3(texCoords.x, texCoords.y + 1, mipLevel)).r;
        
        depth = min(depth, min(depthRight, depthDown));
    }

    depthOut[texCoords] = depth;
}