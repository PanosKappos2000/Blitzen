#include "dx12Data.h"
#include "Renderer/blitRenderingResources.h"

namespace BlitzenDX12
{
    class Dx12Renderer
    {
    public:

        Dx12Renderer();

        uint8_t Init(uint32_t windowWidth, uint32_t windowHeight);

        void SetupResourceManagement();

        uint8_t UploadTexture(BlitzenEngine::DDS_HEADER& header, BlitzenEngine::DDS_HEADER_DXT10& header10,
        void* pData, const char* filepath);

        uint8_t SetupForRendering(BlitzenEngine::RenderingResources* pResources, 
        float& pyramidWidth, float& pyramidHeight);

        void DrawFrame(BlitzenEngine::DrawContext& context);

        void Shutdown();

        ~Dx12Renderer();

    public:
        IDXGIFactory4* factory;

        ID3D12Device* device;

    private:
        ID3D12Debug1* m_debugController;

        IDXGIAdapter1* m_adapter;

        ID3D12DebugDevice* m_debugDevice;

        Swapchain m_swapchain;

    private:
        struct FrameTools
        {
            Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
            ID3D12CommandAllocator* commandAllocator;

            ID3D12Fence* inFlightFence;
        };
        BlitCL::StaticArray<FrameTools, ce_framesInFlight> m_frameTools;

    private:

        Dx12Stats m_stats;

        uint32_t m_currentFrame = 0;
    };


    uint8_t PickAdapter(IDXGIAdapter1* adapter, ID3D12Device** ppDevice, IDXGIFactory4* factory);

    uint8_t ValidateAdapter(IDXGIAdapter1* adapter, ID3D12Device** ppDevice);

    uint8_t CreateSwapchain(Swapchain& swapchain, uint32_t windowWidth, uint32_t windowHeight, 
    IDXGIFactory4* factory, ID3D12CommandQueue* commandQueue);
}