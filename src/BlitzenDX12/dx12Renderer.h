#include "dx12Data.h"
#include "Renderer/blitRenderingResources.h"

namespace BlitzenDX12
{
    class Dx12Renderer
    {
    public:
        Dx12Renderer();

        uint8_t Init();

        void SetupResourceManagement();

        uint8_t SetupForRendering(BlitzenEngine::RenderingResources* pResources);
    public:
        IDXGIFactory4* factory;

        ID3D12Device* device;

    private:
        ID3D12Debug1* m_debugController;

        IDXGIAdapter1* m_adapter;

        ID3D12DebugDevice* m_debugDevice;

    private:
        struct FrameTools
        {
            ID3D12CommandQueue* commandQueue;
            ID3D12CommandAllocator* commandAllocator;
        };
        BlitCL::StaticArray<FrameTools, ce_framesInFlight> m_frameTools;

    private:

        Dx12Stats m_stats;
    };


    uint8_t PickAdapter(IDXGIAdapter1* adapter, ID3D12Device** ppDevice, IDXGIFactory4* factory);

    uint8_t ValidateAdapter(IDXGIAdapter1* adapter, ID3D12Device** ppDevice);
}