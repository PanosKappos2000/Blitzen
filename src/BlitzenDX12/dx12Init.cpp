#include "dx12Renderer.h"


namespace BlitzenDX12
{
    Dx12Renderer::Dx12Renderer()
    {

    }

    uint8_t Dx12Renderer::Init(uint32_t windowWidth, uint32_t windowHeight)
    {
        uint32_t dxgiFactoryFlags = 0;
        if(ce_bDebugController)
        {
            ID3D12Debug* dc;

            // Probably shouldn't do this for debug functionality
            if(D3D12GetDebugInterface(IID_PPV_ARGS(&dc)) != S_OK)
                return 0;

            if(dc->QueryInterface(IID_PPV_ARGS(&m_debugController)) != S_OK)
                return 0;

            m_debugController->EnableDebugLayer();
            m_debugController->SetEnableGPUBasedValidation(true);

            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
            dc->Release();
            dc = nullptr;
        }

        if(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)) != S_OK)
            return 0;

        if(!PickAdapter(m_adapter, &device, factory))
            return 0;

        if(ce_bDebugController)
            device->QueryInterface(&m_debugDevice);

        BLIT_INFO("DX12 success")
        return 1;
    }

    uint8_t PickAdapter(IDXGIAdapter1* adapter, ID3D12Device** ppDevice, IDXGIFactory4* factory)
    {
        uint32_t adapterIndex = 0;
        while(factory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            // Only querries for discrete GPUs for now
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                ++adapterIndex;
                continue;
            }
            
            if(ValidateAdapter(adapter, ppDevice))
                return 1;

            adapter->Release();
            ++adapterIndex;
        }

        return 0;
    }

    uint8_t ValidateAdapter(IDXGIAdapter1* adapter, ID3D12Device** ppDevice)
    {
        return SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(ppDevice)));
    }

    void Dx12Renderer::Shutdown()
    {

    }

    Dx12Renderer::~Dx12Renderer()
    {

    }
}