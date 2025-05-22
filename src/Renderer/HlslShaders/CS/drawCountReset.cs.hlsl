RWBuffer<uint> rwb_drawCmdCounter : register(u1);

[numthreads(1, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    rwb_drawCmdCounter[0] = 0;
}