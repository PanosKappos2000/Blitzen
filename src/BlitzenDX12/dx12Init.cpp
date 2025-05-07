#include "dx12Renderer.h"
#include "dx12Commands.h"
#include "Platform/platform.h"
#include "dx12Resources.h"
#include "dx12Pipelines.h"


namespace BlitzenDX12
{
	Dx12Renderer* Dx12Renderer::s_pThis = nullptr;

    Dx12Renderer::Dx12Renderer()
    {
        s_pThis = this;
    }

    static uint8_t CreateFactory(IDXGIFactory6** ppFactory, Microsoft::WRL::ComPtr<ID3D12Debug>& debugController)
    {
		UINT dxgiFactoryFlags = 0;
        if constexpr (ce_bDebugController)
        {
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.ReleaseAndGetAddressOf()))))
            {
                debugController->EnableDebugLayer();
				dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
            }
            else
            {
				BLIT_ERROR("Failed to create debug controller");
            }
        }

        return SUCCEEDED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(ppFactory)));
    }

    static uint8_t ChooseAdapter(IDXGIFactory6* factory, IDXGIAdapter4** ppAdapter)
    {
        return SUCCEEDED(factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(ppAdapter)));
        // TODO: Consider a fallback with IDXGIAdapter1 and manual adapter selction
    }

    static uint8_t CreateDevice(IDXGIAdapter4* adapter, ID3D12Device** ppDevice)
    {
        return SUCCEEDED(D3D12CreateDevice(adapter, Ce_DeviceFeatureLevel, IID_PPV_ARGS(ppDevice)));
    }

    static uint8_t CreateDebugController(ID3D12Debug* pDebugController, Microsoft::WRL::ComPtr<ID3D12Debug1>& debugController1, 
        ID3D12Device* device)
    {
        if (SUCCEEDED(pDebugController->QueryInterface(IID_PPV_ARGS(debugController1.ReleaseAndGetAddressOf()))))
        {
            debugController1->SetEnableGPUBasedValidation(TRUE);
            if (!CheckForDeviceRemoval(device))
            {
                BLIT_ERROR("Device removed");
                return 0;
            }
        }
        else
        {
            return 0;
        }

        return 1;
    }

    uint8_t Dx12Renderer::Init(uint32_t windowWidth, uint32_t windowHeight, HWND hwnd)
    {
        #if !defined(BLIT_DOUBLE_BUFFERING)
            BLIT_WARN("Double buffering enabled by default for Dx12");
        #endif

        if (!CreateFactory(&m_factory, m_debugController))
        {
			BLIT_ERROR("Failed to create DXGI factory");
            return 0;
        }

        if (!ChooseAdapter(m_factory.Get(), m_chosenAdapter.ReleaseAndGetAddressOf()))
        {
            BLIT_ERROR("Failed to choose adapter");
            return 0;
        }

        if (!CreateDevice(m_chosenAdapter.Get(), m_device.ReleaseAndGetAddressOf()))
        {
            BLIT_ERROR("Failed to create device");
            return 0;
        }
		if (!m_device)
		{
			BLIT_ERROR("Device is null");
			return 0;
		}
		if (!CheckForDeviceRemoval(m_device.Get()))
		{
			BLIT_ERROR("Device removed");
			return 0;
		}

		if (Ce_GPUValidationRequested)
		{
            if (!CreateDebugController(m_debugController.Get(), m_debugController1, m_device.Get()))
            {
				BLIT_ERROR("Failed to create debug controller 1");
                return 0;
            }
		}
        //TODO: Think about adding ID3D12DebugDevice or whatever it is called to watchout for unfreed resources

		if (!CreateCommandQueue(m_device.Get(), m_commandQueue.ReleaseAndGetAddressOf(),
            D3D12_COMMAND_QUEUE_FLAG_NONE, D3D12_COMMAND_LIST_TYPE_DIRECT))
		{
			BLIT_ERROR("Failed to create command queue");
			return 0;
		}

        if (!CreateCommandQueue(m_device.Get(), m_transferCommandQueue.ReleaseAndGetAddressOf(),
            D3D12_COMMAND_QUEUE_FLAG_NONE, D3D12_COMMAND_LIST_TYPE_COPY))
        {
            BLIT_ERROR("Failed to create transfer command queue");
            return 0;
        }

		for (uint32_t i = 0; i < ce_framesInFlight; i++)
		{
			if (!m_frameTools[i].Init(m_device.Get()))
			{
				BLIT_ERROR("Failed to create frame tools");
				return 0;
			}
		}

		if (!CreateSwapchain(m_factory.Get(), m_commandQueue.Get(), windowWidth, windowHeight, 
            hwnd, &m_swapchain))
		{
			BLIT_ERROR("Failed to create swapchain");
            if (CheckForDeviceRemoval(m_device.Get()))
            {
                BLIT_INFO("Device was not removed");
            }
			return 0;
		}
		m_swapchainWidth = windowWidth;
		m_swapchainHeight = windowHeight;

        if (!CreateDescriptorHeaps(m_device.Get(), m_rtvHeap.ReleaseAndGetAddressOf(), m_srvHeap.ReleaseAndGetAddressOf(), m_dsvHeap.ReleaseAndGetAddressOf(), 
            m_samplerHeap.ReleaseAndGetAddressOf()))
        {
            BLIT_ERROR("Failed to create descriptor heaps");
            return 0;
        }
        m_descriptorContext.srvHandle = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
        m_descriptorContext.srvIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


		if (!CreateSwapchainResources(m_swapchain.Get(), m_device.Get(), m_swapchainBackBuffers, m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
            m_descriptorContext))
		{
			BLIT_ERROR("Failed to create swapchain back buffers");
			return 0;
		}

        if (!CreateDepthTargets(m_device.Get(), m_depthBuffers, m_dsvHeap->GetCPUDescriptorHandleForHeapStart(),
            m_descriptorContext, m_swapchainWidth, m_swapchainHeight))
        {
            BLIT_ERROR("Failed to create swapchain back buffers");
            return 0;
        }

		if (!CreateTriangleGraphicsPipeline(m_device.Get(), m_triangleRootSignature, m_trianglePso.ReleaseAndGetAddressOf()))
		{
			BLIT_ERROR("Failed to create triangle graphics pipeline");
			return 0;
		}

        return 1;
    }

    uint8_t CreateSwapchain(IDXGIFactory6* factory, ID3D12CommandQueue* queue, uint32_t windowWidth, uint32_t windowHeight, 
        HWND hWnd, Microsoft::WRL::ComPtr <IDXGISwapChain3>* pSwapchain)
    {
        DXGI_SWAP_CHAIN_DESC1 scDesc = {};
        scDesc.BufferCount = ce_framesInFlight;
        scDesc.Width = windowWidth;
        scDesc.Height = windowHeight;
        scDesc.Format = Ce_SwapchainFormat;
        scDesc.BufferUsage = Ce_SwapchainBufferUsage;
        scDesc.SwapEffect = Ce_SwapchainSwapEffect;
        scDesc.SampleDesc.Count = 1;// No mutlisampling for now, so this is hardcoded
        scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;// No reason to ever change this for a 3D renderer
        scDesc.Scaling = DXGI_SCALING_STRETCH;
        scDesc.Stereo = FALSE;// Only relevant for stereoscopic 3D rendering... so no

        // Extra protection settings
        BOOL allowTearing = FALSE;
        factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        if (allowTearing)
        {
            scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }
        else
        {
            scDesc.Flags = 0;
        }

        Microsoft::WRL::ComPtr<IDXGISwapChain1> tempSwapchain;
        // Creates the swapchain
        auto swapchainRes{ factory->CreateSwapChainForHwnd(queue, hWnd, &scDesc, nullptr, nullptr, &tempSwapchain) };
		if (FAILED(swapchainRes))
		{
			return LOG_ERROR_MESSAGE_AND_RETURN(swapchainRes);
		}
        // Casted to IDXGISwapChain3
        auto swapchainCastRest = tempSwapchain.As(pSwapchain);
        if (FAILED(swapchainCastRest))
        {
			return LOG_ERROR_MESSAGE_AND_RETURN(swapchainCastRest);
        }

        // success
        return 1;
    }

    Dx12Renderer::~Dx12Renderer()
    {
        auto& frameTools = m_frameTools[m_currentFrame];

        const UINT64 fence = frameTools.inFlightFenceValue;
        m_commandQueue->Signal(frameTools.inFlightFence.Get(), fence);
        frameTools.inFlightFenceValue++;
        // Waits for the previous frame
        if (frameTools.inFlightFence->GetCompletedValue() < fence)
        {
            frameTools.inFlightFence->SetEventOnCompletion(fence, frameTools.inFlightFenceEvent);
            WaitForSingleObject(frameTools.inFlightFenceEvent, INFINITE);
        }
    }
}