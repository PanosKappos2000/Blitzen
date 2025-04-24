#include "dx12Data.h"
#include "Renderer/blitRenderingResources.h"
#include "Renderer/blitDDSTextures.h"
#include "Core/blitLogger.h"

namespace BlitzenDX12
{
    class Dx12Renderer
    {
    public:

        Dx12Renderer();
        ~Dx12Renderer();

        Dx12Renderer(const Dx12Renderer& dx) = delete;
        Dx12Renderer operator = (const Dx12Renderer& dx) = delete;
    
        // API initialization (after this it should have the data and handles to do simple things)
        uint8_t Init(uint32_t windowWidth, uint32_t windowHeight);
    
        // Synchronizes the backend with the loaded resources and sets it up for GPU driven rendering
        uint8_t SetupForRendering(BlitzenEngine::RenderingResources* pResources, float& pyramidWidth, float& pyramidHeight);
    
        // Function for DDS texture loading
        uint8_t UploadTexture(void* pData, const char* filepath);
    
        // Shows a loading screen while waiting for resources to be loaded
        void DrawWhileWaiting();
    
        void SetupWhileWaitingForPreviousFrame(const BlitzenEngine::DrawContext& context);
    
        // Called each frame to draw the scene that is requested by the engine
        void DrawFrame(BlitzenEngine::DrawContext& context);
    
        // When a dynamic object moves, it should call this function to update the staging buffer
        void UpdateObjectTransform(uint32_t trId, BlitzenEngine::MeshTransform& newTr);

        inline static Dx12Renderer* GetRendererInstance() { return s_pThis; }

    public:
        Microsoft::WRL::ComPtr<IDXGIFactory2> m_factory;

        Microsoft::WRL::ComPtr<ID3D12Device> m_device;

    private:

        Microsoft::WRL::ComPtr<IDXGIAdapter> m_adapter;

        ID3D12DebugDevice* m_debugDevice;

        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;

        Swapchain m_swapchain;

    private:
        struct FrameTools
        {
            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;

            Microsoft::WRL::ComPtr<ID3D12CommandList> commandList;

            Microsoft::WRL::ComPtr<ID3D12Fence> inFlightFence;
        };
        BlitCL::StaticArray<FrameTools, ce_framesInFlight> m_frameTools;

    private:

        Dx12Stats m_stats;

        uint32_t m_currentFrame = 0;
    private:

		static Dx12Renderer* s_pThis;
    };


    uint8_t CreateFactory(IDXGIFactory2** ppFactory);

    uint8_t GetAdapterFromFactory(IDXGIFactory2* pFactory, IDXGIAdapter** ppAdapter);

    uint8_t CreateCommandQueue(ID3D12CommandQueue** ppQueue, ID3D12Device* pDevice);

    uint8_t CreateSwapchain(Swapchain& swapchain, uint32_t windowWidth, uint32_t windowHeight, 
    IDXGIFactory2* factory, ID3D12CommandQueue* commandQueue, ID3D12Device* pDevice);
}