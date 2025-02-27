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
        Microsoft::WRL::ComPtr<IDXGIFactory2> m_factory;

        Microsoft::WRL::ComPtr<ID3D12Device> m_device;

    private:

        Microsoft::WRL::ComPtr<IDXGIAdapter> m_adapter;

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


    uint8_t CreateFactory(IDXGIFactory2** ppFactory);

    uint8_t GetAdapterFromFactory(IDXGIFactory2* pFactory, IDXGIAdapter** ppAdapter);

    uint8_t CreateSwapchain(Swapchain& swapchain, uint32_t windowWidth, uint32_t windowHeight, 
    IDXGIFactory2* factory, ID3D12CommandQueue* commandQueue);
}