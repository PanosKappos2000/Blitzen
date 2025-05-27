#if defined(_WIN32)

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

	uint8_t CreateSwapchainResources(IDXGISwapChain3* swapchain, ID3D12Device* device, DX12WRAPPER<ID3D12Resource>* backBuffers, DescriptorContext& ctx)
	{
		for (UINT i = 0; i < ce_framesInFlight; i++)
		{
			ctx.m_swapchainRtvOffset[i] = ctx.m_rtvHeapOffset;
			ctx.m_swapchainRtvHandle[i] = ctx.m_rtvHeapHandle;
			ctx.m_swapchainRtvHandle[i].ptr += ctx.m_swapchainRtvOffset[i] * ctx.m_rtvHeapIncrement;

			HRESULT getBackBufferResult = swapchain->GetBuffer(i, IID_PPV_ARGS(backBuffers[i].GetAddressOf()));
			if (FAILED(getBackBufferResult))
			{
				return LOG_ERROR_MESSAGE_AND_RETURN(getBackBufferResult);
			}

			CreateRenderTargetView(device, ctx, Ce_SwapchainFormat, D3D12_RTV_DIMENSION_TEXTURE2D, backBuffers[i].Get());
		}

		// Success
		return 1;
	}

	uint8_t CreateDepthTargets(ID3D12Device* device, DX12WRAPPER<ID3D12Resource>* depthBuffers, DescriptorContext& ctx, uint32_t swapchainWidth, uint32_t swapchainHeight)
	{
		for (UINT i = 0; i < ce_framesInFlight; i++)
		{
			ctx.m_depthTargetDsvOffset[i] = ctx.m_dsvHeapOffset;
			ctx.m_depthTargetDSVHandle[i] = ctx.m_dsvHeapHandle;
			ctx.m_depthTargetDSVHandle[i].ptr += ctx.m_depthTargetDsvOffset[i] * ctx.m_dsvHeapIncrement;

			D3D12_CLEAR_VALUE clear{};
			clear.Format = Ce_DepthTargetFormat;
			clear.DepthStencil.Depth = Ce_ClearDepth;

			HRESULT resourceRes{ CreateImageResource(device, depthBuffers[i].GetAddressOf(), swapchainWidth, swapchainHeight, 1,
				Ce_DepthTargetFormat, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear) };
			if (FAILED(resourceRes))
			{
				return LOG_ERROR_MESSAGE_AND_RETURN(resourceRes);
			}

			D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc{};
			viewDesc.Format = Ce_DepthTargetFormat;
			viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

			auto handle{ ctx.m_dsvHeapHandle };
			handle.ptr += ctx.m_dsvHeapOffset * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

			// Creates view
			device->CreateDepthStencilView(depthBuffers[i].Get(), &viewDesc, handle);

			// Increments the offset
			ctx.m_dsvHeapOffset++;
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

	void CreateDepthPyramidDescriptors(ID3D12Device* device, Dx12Renderer::VarBuffers* pBuffers, DescriptorContext& context,
		DX12WRAPPER<ID3D12Resource>* pDepthTargets, UINT drawWidth, UINT drawHeight)
	{
		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			auto& var = pBuffers[i];

			context.m_HI_Z_MapSRVOffset[i] = context.m_viewHeapCurrentOffset;
			context.m_HI_Z_MapSRVHandle[i] = context.m_viewHeapHandle;
			context.m_HI_Z_MapSRVHandle[i].ptr += context.m_HI_Z_MapSRVOffset[i] * context.m_viewHeapIncrement;

			CreateTexture2DShaderResourceView(device, var.depthPyramid.pyramid.Get(), context, Ce_DepthPyramidFormat, var.depthPyramid.mipCount);
		}

		for (uint32_t f = 0; f < ce_framesInFlight; ++f)
		{
			auto& var = pBuffers[f];

			context.m_HI_Z_MapMipsFirstUAVOffset[f] = context.m_viewHeapCurrentOffset;
			context.m_HI_Z_MapMipsFirstUAVHandle[f] = context.m_viewHeapHandle;
			context.m_HI_Z_MapMipsFirstUAVHandle[f].ptr += context.m_HI_Z_MapMipsFirstUAVOffset[f] * context.m_viewHeapIncrement;

			SIZE_T mipsEndOffset = context.m_viewHeapCurrentOffset;
			auto mipsStartHandle = context.m_HI_Z_MapMipsFirstUAVHandle[f];


			for (uint32_t i = 0; i < Ce_DepthPyramidMaxMips; ++i)
			{
				var.depthPyramid.mips[i] = mipsStartHandle;
				var.depthPyramid.mips[i].ptr += i * context.m_viewHeapIncrement;
				mipsEndOffset++;
			}
			
			for (uint32_t i = 0; i < var.depthPyramid.mipCount; ++i)
			{
				Create2DTextureUnorderedAccessView(device, context, var.depthPyramid.pyramid.Get(), Ce_DepthPyramidFormat, i);
			}

			context.m_viewHeapCurrentOffset = mipsEndOffset;
		}
		

		for (size_t i = 0; i < ce_framesInFlight; ++i)
		{
			context.m_depthTargetSRVOffset[i] = context.m_viewHeapCurrentOffset;
			context.m_depthTargetSRVHandle[i] = context.m_viewHeapHandle;
			context.m_depthTargetSRVHandle[i].ptr += context.m_depthTargetSRVOffset[i] * context.m_viewHeapIncrement;

			CreateTexture2DShaderResourceView(device, pDepthTargets[i].Get(), context, Ce_DepthTargetSRVFormat, 1);
		}
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

#endif