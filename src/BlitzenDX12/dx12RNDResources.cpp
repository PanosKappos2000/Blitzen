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
		if (allowTearing && Ce_SwapchainSwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD)
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

	uint8_t CreateSwapchainResources(IDXGISwapChain3* swapchain, ID3D12Device* device, DX12WRAPPER<ID3D12Resource>* backBuffers,
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle, SIZE_T& rtvHeapOffset)
	{
		for (UINT i = 0; i < ce_framesInFlight; i++)
		{
			auto getBackBufferResult = swapchain->GetBuffer(i, IID_PPV_ARGS(backBuffers[i].GetAddressOf()));
			if (FAILED(getBackBufferResult))
			{
				return LOG_ERROR_MESSAGE_AND_RETURN(getBackBufferResult);
			}
			CreateRenderTargetView(device, Ce_SwapchainFormat, D3D12_RTV_DIMENSION_TEXTURE2D, backBuffers[i].Get(), rtvHeapHandle, rtvHeapOffset);
		}

		// Success
		return 1;
	}

	uint8_t CreateDepthTargets(ID3D12Device* device, DX12WRAPPER<ID3D12Resource>* depthBuffers, D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapHandle,
		SIZE_T& dsvHeapOffset, uint32_t swapchainWidth, uint32_t swapchainHeight)
	{
		for (UINT i = 0; i < ce_framesInFlight; i++)
		{
			D3D12_CLEAR_VALUE clear{};
			clear.Format = Ce_DepthTargetFormat;
			clear.DepthStencil.Depth = Ce_ClearDepth;
			auto resourceRes{ CreateImageResource(device, depthBuffers[i].GetAddressOf(), swapchainWidth, swapchainHeight, 1,
				Ce_DepthTargetFormat, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear) };
			if (FAILED(resourceRes))
			{
				return LOG_ERROR_MESSAGE_AND_RETURN(resourceRes);
			}

			D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc{};
			viewDesc.Format = Ce_DepthTargetFormat;
			viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			// Creates the view with the right offset
			dsvHeapHandle.ptr += dsvHeapOffset * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			device->CreateDepthStencilView(depthBuffers[i].Get(), &viewDesc, dsvHeapHandle);
			// Increments the offset
			dsvHeapOffset++;
		}

		// success
		return 1;
	}

	uint8_t CreateDepthPyramidResource(ID3D12Device* device, DepthPyramid& depthPyramid, uint32_t width, uint32_t height)
	{
		// Conservative starting extent
		//depthPyramid.width = BlitML::PreviousPow2(width);
		//depthPyramid.height = BlitML::PreviousPow2(height);

		// NOTE: THIS IS DIFFERENT FROM VULKAN BECAUSE THE SHADER GENERATION LOGIC IS DIFFERENT
		depthPyramid.width = BlitML::Max(1u, (width) >> 1);
		depthPyramid.height = BlitML::Max(1u, (height) >> 1);

		depthPyramid.mipCount = BlitML::GetDepthPyramidMipLevels(depthPyramid.width, depthPyramid.height);

		// Image resource
		if (!CreateImageResource(device, depthPyramid.pyramid.ReleaseAndGetAddressOf(), depthPyramid.width, depthPyramid.height,
			depthPyramid.mipCount, Ce_DepthPyramidFormat, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_HEAP_TYPE_DEFAULT,
			D3D12_RESOURCE_STATE_COMMON, nullptr))
		{
			BLIT_ERROR("Failed to create depth pyramid resource");
			return 0;
		}

		return 1;
	}

	void CreateDepthPyramidDescriptors(ID3D12Device* device, Dx12Renderer::VarBuffers* pBuffers, Dx12Renderer::DescriptorContext& context,
		ID3D12DescriptorHeap* srvHeap, DX12WRAPPER<ID3D12Resource>* pDepthTargets, UINT drawWidth, UINT drawHeight, ID3D12DescriptorHeap* samplerHeap)
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
				context.srvHeapOffset, Ce_DepthTargetSRVFormat, 1);
		}

		context.depthPyramidSamplerOffset = context.samplerHeapOffset;
		context.depthPyramidSamplerHandle = context.samplerHandle;
		context.depthPyramidSamplerHandle.ptr += context.depthPyramidSamplerOffset * context.samplerIncrementSize;
		CreateSampler(device, samplerHeap->GetCPUDescriptorHandleForHeapStart(), context.samplerHeapOffset, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, nullptr, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
	}

	void CopyDepthPyramidToSwapchain(ID3D12GraphicsCommandList4* commandList, ID3D12Resource* swapchainBackBuffer, ID3D12Resource* depthPyramid,
		UINT depthPyramidWidth, UINT depthPyramidHeight, ID3D12DescriptorHeap* descriptorHeap, ID3D12CommandQueue* queue, IDXGISwapChain3* swapchain, 
		uint32_t pyramidMip, uint32_t swapchainWidht, uint32_t swapchainHeight)
	{
		D3D12_RESOURCE_BARRIER barrier[2] {};
		CreateResourcesTransitionBarrier(barrier[0], depthPyramid, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_COPY_SOURCE);
		CreateResourcesTransitionBarrier(barrier[1], swapchainBackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_COPY_DEST);
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(barrier), barrier);

		D3D12_TEXTURE_COPY_LOCATION srcLocation {};
		srcLocation.pResource = depthPyramid;
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		srcLocation.SubresourceIndex = pyramidMip;

		D3D12_TEXTURE_COPY_LOCATION destLocation {};
		destLocation.pResource = swapchainBackBuffer;
		destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		destLocation.SubresourceIndex = 0;

		// Get the width and height for the current mip level
		uint32_t mipWidth = max(1u, (depthPyramidWidth >> pyramidMip));
		uint32_t mipHeight = max(1u, (depthPyramidHeight >> pyramidMip));

		// Copy region parameters
		D3D12_BOX copyRegion = {};
		copyRegion.left = 0;
		copyRegion.top = 0;
		copyRegion.front = 0;
		copyRegion.right = mipWidth;  // Width of the mip
		copyRegion.bottom = mipHeight;  // Height of the mip
		copyRegion.back = 1;

		commandList->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, &copyRegion);

		// Transition the swapchain back buffer back to present state
		D3D12_RESOURCE_BARRIER presentBarrier {};
		CreateResourcesTransitionBarrier(presentBarrier, swapchainBackBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);

		commandList->ResourceBarrier(1, &presentBarrier);

		D3D12_RESOURCE_BARRIER pyramidBarrier{};
		CreateResourcesTransitionBarrier(pyramidBarrier, depthPyramid, D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->ResourceBarrier(1, &pyramidBarrier);

		commandList->Close();
		ID3D12CommandList* commandLists[] = { commandList };
		queue->ExecuteCommandLists(1, commandLists);

		swapchain->Present(1, 0);
	}
}