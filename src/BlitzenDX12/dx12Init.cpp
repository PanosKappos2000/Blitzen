#include "dx12Renderer.h"
#include "Engine/blitzenEngine.h"


namespace BlitzenDX12
{
	Dx12Renderer* Dx12Renderer::s_pThis = nullptr;

    Dx12Renderer::Dx12Renderer()
    {
        s_pThis = this;
    }

    static void SetupResourceManagement()
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
        m_factory.Get(), m_commandQueue.Get(), m_device.Get()))
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

    uint8_t CreateSwapchain(Swapchain& swapchain, uint32_t windowWidth, uint32_t windowHeight, 
    IDXGIFactory2* factory, ID3D12CommandQueue* commandQueue, ID3D12Device* pDevice)
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

        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.NumDescriptors = ce_framesInFlight; 
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.NodeMask = 0;
		if(FAILED(pDevice->CreateDescriptorHeap(&rtvHeapDesc, 
        IID_PPV_ARGS(swapchain.m_renderTargetViewDescriptor.GetAddressOf()))))
            return 0;
		swapchain.m_heapIncrement = pDevice->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);
        
        DXGI_SWAP_CHAIN_DESC1 description = {};
		description.Width = windowWidth;
		description.Height = windowHeight;
		description.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		description.Stereo = false;
		description.SampleDesc = { 1,0 };
		description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		description.BufferCount = ce_framesInFlight;
		description.Scaling = DXGI_SCALING_NONE;
		description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		description.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		description.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        HWND hwnd = reinterpret_cast<HWND>(BlitzenPlatform::GetWindowHandle());
    
        // Validates HWND
        if (!IsWindow(hwnd))
            return 0;
    
        // Validates Command Queue
        if (!commandQueue)
            return 0;
    
        if(FAILED(factory->CreateSwapChainForHwnd(commandQueue, hwnd, 
        &description, nullptr, nullptr, swapchain.handle.GetAddressOf())))
            return 0;
        
        return 1;
    }

    Dx12Renderer::~Dx12Renderer()
    {
        
    }
}