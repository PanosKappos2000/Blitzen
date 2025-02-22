#include "dx12Data.h"

namespace BlitzenDX12
{
    class Dx12Renderer
    {
    public:
        Dx12Renderer();

        uint8_t Init();
    public:
        IDXGIFactory4* factory;

        ID3D12Device* device;

    private:
        ID3D12Debug1* m_debugController;

        IDXGIAdapter1* m_adapter;
    };


    uint8_t PickAdapter(IDXGIAdapter1* adapter, ID3D12Device* device, IDXGIFactory4* factory);

    uint8_t ValidateAdapter(IDXGIAdapter1* adapter, ID3D12Device* device);
}