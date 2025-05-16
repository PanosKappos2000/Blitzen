#include "dx12RNDResources.h"

namespace BlitzenDX12
{
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

	void CreateDepthPyramidDescriptors(ID3D12Device* device, Dx12Renderer::VarBuffers* pBuffers, Dx12Renderer::DescriptorContext& context,
		ID3D12DescriptorHeap* srvHeap, DX12WRAPPER<ID3D12Resource>* pDepthTargets, UINT drawWidth, UINT drawHeight)
	{
		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			auto& var = pBuffers[i];

			context.depthPyramidSrvOffset[i] = context.srvHeapOffset;
			context.depthPyramidSrvHandle[i] = context.srvHandle;
			context.depthPyramidSrvHandle[i].ptr += context.depthPyramidSrvOffset[i] * context.srvIncrementSize;

			CreateTextureShaderResourceView(device, var.depthPyramid.pyramid.Get(), srvHeap->GetCPUDescriptorHandleForHeapStart(),
				context.srvHeapOffset, Ce_DepthPyramidFormat, var.depthPyramid.mipCount);
		}

		for (uint32_t f = 0; f < ce_framesInFlight; ++f)
		{
			auto& var = pBuffers[f];

			context.depthPyramidMipsSrvOffset[f] = context.srvHeapOffset;
			context.depthPyramidMipsSrvHandle[f] = context.srvHandle;
			context.depthPyramidMipsSrvHandle[f].ptr += context.depthPyramidMipsSrvOffset[f] * context.srvIncrementSize;

			auto mipHandleStart = context.depthPyramidMipsSrvHandle[f];
			for (uint32_t i = 0; i < var.depthPyramid.mipCount; ++i)
			{
				Create2DTextureUnorderedAccessView(device, var.depthPyramid.pyramid.Get(), Ce_DepthPyramidFormat, i, context.srvHeapOffset,
					srvHeap->GetCPUDescriptorHandleForHeapStart());

				var.depthPyramid.mips[i] = mipHandleStart;
				var.depthPyramid.mips[i].ptr += i * context.srvIncrementSize;
			}
		}

		for (size_t i = 0; i < ce_framesInFlight; ++i)
		{
			context.depthTargetSrvOffset[i] = context.srvHeapOffset;
			context.depthTargetSrvHandle[i] = context.srvHandle;
			context.depthTargetSrvHandle[i].ptr += context.depthTargetSrvOffset[i] * context.srvIncrementSize;

			CreateTextureShaderResourceView(device, pDepthTargets[i].Get(), srvHeap->GetCPUDescriptorHandleForHeapStart(),
				context.srvHeapOffset, Ce_DepthTargetSrvFormat, 1);
		}
	}

	void RecreateDepthPyramidDescriptors(ID3D12Device* device, Dx12Renderer::VarBuffers* pBuffers, Dx12Renderer::DescriptorContext& context,
		ID3D12DescriptorHeap* srvHeap, SIZE_T srvOffsetForPyramidDescriptors, DX12WRAPPER<ID3D12Resource>* pDepthTargets, UINT drawWidth, UINT drawHeight)
	{
		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			auto& var = pBuffers[i];

			CreateTextureShaderResourceView(device, var.depthPyramid.pyramid.Get(), srvHeap->GetCPUDescriptorHandleForHeapStart(),
				srvOffsetForPyramidDescriptors, Ce_DepthPyramidFormat, var.depthPyramid.mipCount);
		}

		for (uint32_t f = 0; f < ce_framesInFlight; ++f)
		{
			auto& var = pBuffers[f];

			for (uint32_t i = 0; i < var.depthPyramid.mipCount; ++i)
			{
				Create2DTextureUnorderedAccessView(device, var.depthPyramid.pyramid.Get(), Ce_DepthPyramidFormat, i, srvOffsetForPyramidDescriptors,
					srvHeap->GetCPUDescriptorHandleForHeapStart());
			}
		}

		for (size_t i = 0; i < ce_framesInFlight; ++i)
		{
			CreateTextureShaderResourceView(device, pDepthTargets[i].Get(), srvHeap->GetCPUDescriptorHandleForHeapStart(),
				srvOffsetForPyramidDescriptors, Ce_DepthTargetSrvFormat, 1);
		}
	}
}