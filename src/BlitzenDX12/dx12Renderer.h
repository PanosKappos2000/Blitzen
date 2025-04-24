#pragma once
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
        uint8_t Init(HWND hwnd, uint32_t windowWidth, uint32_t windowHeight);
    
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
        struct FrameTools
        {
            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mainGraphicsCommandAllocator;

            Microsoft::WRL::ComPtr<ID3D12CommandList> mainGraphicsCommandList;
            Microsoft::WRL::ComPtr<ID3D12Fence> inFlightFence;

            uint8_t Init(ID3D12Device* device);
            uint64_t inFlightFenceValue;
			HANDLE inFlightFenceEvent;
        };

        Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;

        Microsoft::WRL::ComPtr<ID3D12Device> m_device;

    private:

        Microsoft::WRL::ComPtr<ID3D12Debug> m_debugController;

        Microsoft::WRL::ComPtr<IDXGIAdapter4> m_chosenAdapter;

        Microsoft::WRL::ComPtr<ID3D12Debug1> m_debugController1;

        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;

        Swapchain m_swapchain;

    private:
        BlitCL::StaticArray<FrameTools, ce_framesInFlight> m_frameTools;

    private:

        Dx12Stats m_stats;

        uint32_t m_currentFrame = 0;
    private:

		static Dx12Renderer* s_pThis;
    };

    // Useful helper to check for device removal before calling a function that uses it
    inline uint8_t CheckForDeviceRemoval(ID3D12Device* device)
    {
        auto removalReason = device->GetDeviceRemovedReason();
        if (FAILED(removalReason))
        {
            _com_error err{ removalReason };
            BLIT_FATAL("Device removal reason: %s", err.ErrorMessage());
            return 0;
        }

        // Safe
        return 1;
    }

    // If a dx12 functcion fails, it can calls this to log the result and return 0
    inline uint8_t LOG_ERROR_MESSAGE_AND_RETURN(HRESULT res)
    {
        _com_error err{ res };
        BLIT_ERROR("Dx12Error: %s", err.ErrorMessage());
        return 0;
    }

    uint8_t CreateSwapchain(IDXGIFactory6* factory, ID3D12CommandQueue* queue, uint32_t windowWidth, uint32_t windowHeight, 
        HWND hwnd, Microsoft::WRL::ComPtr <IDXGISwapChain3>* pSwapchain);
}