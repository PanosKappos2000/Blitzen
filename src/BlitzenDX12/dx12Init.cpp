#include "dx12Renderer.h"
#include "Engine/blitzenEngine.h"


namespace BlitzenDX12
{
    Dx12Renderer::Dx12Renderer()
    {

    }

    uint8_t Dx12Renderer::Init(uint32_t windowWidth, uint32_t windowHeight)
    {
        if(!CreateFactory(m_factory.GetAddressOf()))
            return 0;

        if(!GetAdapterFromFactory(m_factory.Get(), m_adapter.GetAddressOf()))
            return 0;

        if(FAILED(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_1, 
        IID_PPV_ARGS(m_device.GetAddressOf()))))
            return 0;

        if(!CreateCommandQueue(m_commandQueue.GetAddressOf(), m_device.Get()))
            return 0;

        SetupResourceManagement();
        if(!m_stats.bResourceManagement)
            return 0;

        if(!CreateSwapchain(m_swapchain, windowWidth, windowHeight, 
        m_factory.Get(), m_commandQueue.Get()))
            return 0;

        return 1;
    }

    uint8_t CreateFactory(IDXGIFactory2** ppFactory)
    {
        UINT dxgiFactoryFlags = 0;

        // Enables debug controller on debug builds
        if(ce_bDebugController)
        {
            //UINT debugFlags = D3D12_DEBUG_FEATURE_ENABLE_GPU_ASSISTED;
            Microsoft::WRL::ComPtr<ID3D12Debug1> dc; 
            if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(dc.GetAddressOf()))))
            {
                dc->EnableDebugLayer();
                dc->SetEnableGPUBasedValidation(true);
                /*dc->SetDebugParameter(D3D12_DEBUG_PARAMETER_NAME_GPU_BASED_VALIDATION_SETTINGS, 
                &debugFlags, sizeof(debugFlags));*/
                dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
            }
        }

        return (SUCCEEDED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(ppFactory))));
    }

    uint8_t GetAdapterFromFactory(IDXGIFactory2* pFactory, IDXGIAdapter** ppAdapter)
    {
        Microsoft::WRL::ComPtr<IDXGIFactory6> fac6;

		if (pFactory->QueryInterface(IID_PPV_ARGS(&fac6)) == S_OK)
        {
			return SUCCEEDED(fac6->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, 
            IID_PPV_ARGS(ppAdapter))); 
		}
        else
        {
            return 0;
        }
    }

    uint8_t CreateCommandQueue(ID3D12CommandQueue** ppQueue, ID3D12Device* pDevice)
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
        queueDesc.NodeMask = 0;

		return SUCCEEDED(pDevice->CreateCommandQueue(&queueDesc, 
        IID_PPV_ARGS(ppQueue)));
    }

    void Dx12Renderer::SetupResourceManagement()
	{
		for (auto& tools : m_frameTools)
		{
			if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, 
            IID_PPV_ARGS(tools.commandAllocator.GetAddressOf()))))
			{
				m_stats.bResourceManagement = 0;
				return;
			}

            if(FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, tools.commandAllocator.Get(), 
            nullptr, IID_PPV_ARGS(tools.commandList.GetAddressOf()))))
            {
				m_stats.bResourceManagement = 0;
				return;
			}

			if(FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, 
            IID_PPV_ARGS(tools.inFlightFence.GetAddressOf()))))
			{
				m_stats.bResourceManagement = 0;
				return;
			}
		}
		m_stats.bResourceManagement = 1;
	}

    uint8_t CreateSwapchain(Swapchain& swapchain, uint32_t windowWidth, uint32_t windowHeight, 
    IDXGIFactory2* factory, ID3D12CommandQueue* commandQueue)
    {
        swapchain.extent.left = 0;
        swapchain.extent.bottom = 0;
        swapchain.extent.right = windowWidth;
        swapchain.extent.top = windowHeight;

        D3D12_VIEWPORT viewport;
        
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        
        viewport.Width = static_cast<float>(windowWidth);
        viewport.Height = static_cast<float>(windowHeight);
        
        viewport.MinDepth = BlitzenEngine::ce_znear;
        viewport.MaxDepth = BlitzenEngine::ce_initialDrawDistance;
        
        if(swapchain.handle != nullptr)
        {
            if(swapchain.handle->ResizeBuffers(2, windowWidth, windowHeight, 
            DXGI_FORMAT_R8G8_B8G8_UNORM, 0) != S_OK)
                return 0;
        }
        else
        {
            DXGI_SWAP_CHAIN_DESC1 swapDesc{};
            swapDesc.BufferCount = 2;// I think this is about double buffering, not sure
            swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapDesc.SampleDesc.Count = 1;
            swapDesc.SampleDesc.Quality = 0;
            swapDesc.Width = windowWidth;
            swapDesc.Height = windowHeight;
            swapDesc.Format = DXGI_FORMAT_R8G8_B8G8_UNORM;// Hardcoding this, I don't need anything else for now
            swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; 
            swapDesc.Scaling = DXGI_SCALING_STRETCH; 
            swapDesc.Stereo = FALSE; 
                    
            Microsoft::WRL::ComPtr<IDXGISwapChain1> newSwapchain;
            HWND hwnd = reinterpret_cast<HWND>(BlitzenPlatform::GetWindowHandle());
    
            // Validates HWND
            if (!IsWindow(hwnd))
                return 0;
    
            // Validates Command Queue
            if (!commandQueue)
                return 0;
    
            if((factory->CreateSwapChainForHwnd(commandQueue, hwnd, 
            &swapDesc, nullptr, nullptr, newSwapchain.GetAddressOf())) == DXGI_ERROR_INVALID_CALL)
                return 0;
    
            newSwapchain.As(&swapchain.handle);
        }
        return 1;
    }

    void Dx12Renderer::Shutdown()
    {

    }

    Dx12Renderer::~Dx12Renderer()
    {
        
    }
}