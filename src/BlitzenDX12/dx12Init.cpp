#include "dx12Renderer.h"
#include "dx12Commands.h"
#include "Platform/platform.h"
#include "dx12Resources.h"
#include "dx12Pipelines.h"
#include "dx12RNDResources.h"


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
        m_descriptorContext.samplerHandle = m_samplerHeap->GetGPUDescriptorHandleForHeapStart();
        m_descriptorContext.samplerIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        m_descriptorContext.rtvIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_descriptorContext.dsvIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);


		if (!CreateSwapchainResources(m_swapchain.Get(), m_device.Get(), m_swapchainBackBuffers, m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
            m_descriptorContext.rtvHeapOffset))
		{
			BLIT_ERROR("Failed to create swapchain back buffers");
			return 0;
		}

        if (!CreateDepthTargets(m_device.Get(), m_depthBuffers, m_dsvHeap->GetCPUDescriptorHandleForHeapStart(),
            m_descriptorContext.dsvHeapOffset, m_swapchainWidth, m_swapchainHeight))
        {
            BLIT_ERROR("Failed to create swapchain back buffers");
            return 0;
        }

        m_descriptorContext.defaultTextureSamplerOffset = m_descriptorContext.samplerHeapOffset;
        m_descriptorContext.defaultTextureSamplerHandle = m_descriptorContext.samplerHandle;
        m_descriptorContext.defaultTextureSamplerHandle.ptr += m_descriptorContext.defaultTextureSamplerOffset * m_descriptorContext.samplerIncrementSize;
        CreateSampler(m_device.Get(), m_samplerHeap->GetCPUDescriptorHandleForHeapStart(), m_descriptorContext.samplerHeapOffset,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, nullptr, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

		if (!CreateTriangleGraphicsPipeline(m_device.Get(), m_triangleRootSignature, m_trianglePso.ReleaseAndGetAddressOf()))
		{
			BLIT_ERROR("Failed to create triangle graphics pipeline");
			return 0;
		}

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