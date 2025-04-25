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
        uint8_t Init(uint32_t windowWidth, uint32_t windowHeight, HWND hwnd);
    
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

            uint64_t inFlightFenceValue;
			HANDLE inFlightFenceEvent;

            uint8_t Init(ID3D12Device* device);
        };

        Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;

        Microsoft::WRL::ComPtr<ID3D12Device> m_device;

    private:

        Microsoft::WRL::ComPtr<ID3D12Debug> m_debugController;
        Microsoft::WRL::ComPtr<ID3D12Debug1> m_debugController1;

        Microsoft::WRL::ComPtr<IDXGIAdapter4> m_chosenAdapter;

        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;

        Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapchain;
        // Swapchain resources
        Microsoft::WRL::ComPtr<ID3D12Resource> m_swapchainBackBuffers [ce_framesInFlight];
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_swapchainRtvHeap;
        D3D12_CPU_DESCRIPTOR_HANDLE m_swapchainRtvHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE m_swapchainRtvHandles [ce_framesInFlight];

    /*
        Descriptors
    */
    private:

    /*
        Pipelines and Root Signatures
    */
    private:

        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_opaqueGraphicsRootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_opaqueGraphicsPipeline;

    private:

        BlitCL::StaticArray<FrameTools, ce_framesInFlight> m_frameTools;

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

/*
// Define descriptor ranges for your large SSBOs (Shader Resource Views)
D3D12_DESCRIPTOR_RANGE range1 = {};
range1.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // Shader Resource View (e.g., SSBO)
range1.NumDescriptors = 1; // Number of descriptors (e.g., 1 large buffer for vertices)
range1.BaseShaderRegister = 0; // The register for the resource (e.g., register 0)
range1.RegisterSpace = 0; // The space for the descriptor

D3D12_DESCRIPTOR_RANGE range2 = {};
range2.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // Another SSBO (e.g., global index buffer)
range2.NumDescriptors = 1;
range2.BaseShaderRegister = 1; // Next register (e.g., register 1)
range2.RegisterSpace = 0;

D3D12_DESCRIPTOR_RANGE range3 = {};
range3.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // Another SSBO (e.g., global transform buffer)
range3.NumDescriptors = 1;
range3.BaseShaderRegister = 2; // Next register
range3.RegisterSpace = 0;

// Define the root parameters (which bind the ranges to the shaders)
D3D12_ROOT_PARAMETER rootParameters[3] = {};

// Set up the root parameter to bind the descriptor ranges to the root signature
rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
rootParameters[0].DescriptorTable.pDescriptorRanges = &range1;

rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
rootParameters[1].DescriptorTable.pDescriptorRanges = &range2;

rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
rootParameters[2].DescriptorTable.pDescriptorRanges = &range3;

// Define the root signature descriptor
D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
rootSignatureDesc.NumParameters = ARRAYSIZE(rootParameters);
rootSignatureDesc.pParameters = rootParameters;
rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;  // Can be used in fixed input assembler stages

// Create the root signature
ComPtr<ID3D12RootSignature> rootSignature;
hr = device->CreateRootSignature(0, &rootSignatureDesc, sizeof(rootSignatureDesc), IID_PPV_ARGS(&rootSignature));
if (FAILED(hr)) {
    std::cerr << "Failed to create root signature!" << std::endl;
    return -1;
}
*/