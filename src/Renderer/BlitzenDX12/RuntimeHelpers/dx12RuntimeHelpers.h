#if defined(_WIN32)
#pragma once
#include "Renderer/BlitzenDX12/Context/dx12Context.h"

namespace BlitzenDX12
{
	void RecreateSwapchain(HWND hwnd, IDXGIFactory6* factory, ID3D12Device* device, ID3D12CommandQueue* queue, uint32_t newWidth, uint32_t newHeight,
		DX12WRAPPER<IDXGISwapChain3>* pSwapchain, DX12WRAPPER<ID3D12Resource>* pSwapchainBuffers, DX12WRAPPER<ID3D12Resource>* pDepthTargets,
		DescriptorContext& descriptorContext, CmdContext* pCmd);

	void RecreateDepthPyramidDescriptors(ID3D12Device* device, ReadWriteResources* rwResourcesArray, DescriptorContext& context, UINT drawWidth, UINT drawHeight);

	void RecreateDepthTargetDescriptor(ID3D12Device* device, DX12WRAPPER<ID3D12Resource>* pDepthTargets, DescriptorContext& ctx);
}
#endif