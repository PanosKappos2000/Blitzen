#include "dx12Renderer.h"
#include "dx12Commands.h"


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

    uint8_t Dx12Renderer::Init(uint32_t windowWidth, uint32_t windowHeight)
    {
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

        return 1;
    }

    uint8_t CheckForDeviceRemoval(ID3D12Device* device)
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

    Dx12Renderer::~Dx12Renderer()
    {
        
    }
}