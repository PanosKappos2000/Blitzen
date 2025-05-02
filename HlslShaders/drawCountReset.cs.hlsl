RWBuffer<uint> drawCountBuffer : register(u1);

[numthreads(1, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    drawCountBuffer[0] = 0;
}