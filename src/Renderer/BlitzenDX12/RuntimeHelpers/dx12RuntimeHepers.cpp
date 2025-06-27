#if defined(_WIN32)
#include "Renderer/BlitzenDX12/Context/dx12Renderer.h"
#include "Renderer/BlitzenDX12/Resources/dx12Pipelines.h"
#include "Renderer/BlitzenDX12/Resources/dx12RNDResources.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"
	
namespace BlitzenDX12
{
	void RecreateSwapchain(HWND hwnd, IDXGIFactory6* factory, ID3D12Device* device, ID3D12CommandQueue* queue, uint32_t newWidth, uint32_t newHeight,
		DX12WRAPPER<IDXGISwapChain3>* pSwapchain, DX12WRAPPER<ID3D12Resource>* pSwapchainBuffers, DX12WRAPPER<ID3D12Resource>* pDepthTargets,
		DescriptorContext& descriptorContext, CmdContext* pCmd)
	{
		// Waits for all frames
		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			auto& cmdContext{ pCmd[i] };
			PlaceFence(cmdContext.m_frameFence.m_value, queue, cmdContext.m_frameFence.m_dx12Handle.Get(), cmdContext.m_frameFence.m_event);

			pSwapchainBuffers[i]->Release();
			pDepthTargets[i]->Release();
		}

		pSwapchain->ReleaseAndGetAddressOf();
		BLIT_ASSERT(CreateSwapchain(factory, queue, newWidth, newHeight, hwnd, pSwapchain));

		// Automatically releases when trying to create. Creates 
		for (UINT i = 0; i < ce_framesInFlight; i++)
		{
			SIZE_T offset{ descriptorContext.m_swapchainRtvOffset[i] };

			HRESULT getBackBufferResult = pSwapchain->Get()->GetBuffer(i, IID_PPV_ARGS(pSwapchainBuffers[i].GetAddressOf()));
			BLIT_ASSERT(SUCCEEDED(getBackBufferResult));

			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = Ce_SwapchainFormat;
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

			// Handle for heap start
			auto handle = descriptorContext.m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
			handle.ptr += offset * descriptorContext.m_rtvHeapIncrement;

			// Creates view
			device->CreateRenderTargetView(pSwapchainBuffers[i].Get(), &rtvDesc, handle);
		}

		for (UINT i = 0; i < ce_framesInFlight; i++)
		{
			SIZE_T offset{ descriptorContext.m_depthTargetDsvOffset[i] };

			D3D12_CLEAR_VALUE clear{};
			clear.Format = Ce_DepthTargetFormat;
			clear.DepthStencil.Depth = Ce_ClearDepth;

			HRESULT depthTargetRes{ CreateImageResource(device, pDepthTargets[i].GetAddressOf(), newWidth, newHeight, 1, Ce_DepthTargetFormat,
				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear) };
			BLIT_ASSERT(SUCCEEDED(depthTargetRes));

			D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc{};
			viewDesc.Format = Ce_DepthTargetFormat;
			viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

			auto handle{ descriptorContext.m_dsvHeap->GetCPUDescriptorHandleForHeapStart() };
			handle.ptr += offset * descriptorContext.m_dsvHeapIncrement;

			device->CreateDepthStencilView(pDepthTargets[i].Get(), &viewDesc, handle);
		}
	}

	void RecreateDepthPyramidDescriptors(ID3D12Device* device, ReadWriteResources* rwResourcesArray, DescriptorContext& context, UINT drawWidth, UINT drawHeight)
	{
		SIZE_T offset{ context.m_HI_Z_MAP_cullOffset[0] };

		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			auto& rwResources = rwResourcesArray[i];

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = Ce_DepthPyramidFormat;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Texture2D.MipLevels = rwResources.m_HI_Z.mipCount;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.PlaneSlice = 0;

			// handle for heap start
			auto handle{ context.m_viewHeap->GetCPUDescriptorHandleForHeapStart() };
			handle.ptr += offset * context.m_viewHeapIncrement;

			// Creates view
			device->CreateShaderResourceView(rwResources.m_HI_Z.pyramid.Get(), &srvDesc, handle);

			// Increments offset
			offset++;
		}

		for (uint32_t f = 0; f < ce_framesInFlight; ++f)
		{
			auto& rwResources = rwResourcesArray[f];
			offset = context.m_HI_Z_MAP_mipOffset[f];

			for (uint32_t hi_z_mip = 0; hi_z_mip < rwResources.m_HI_Z.mipCount; ++hi_z_mip)
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
				uavDesc.Format = Ce_DepthPyramidFormat;
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
				uavDesc.Texture2D.MipSlice = hi_z_mip;
				uavDesc.Texture2D.PlaneSlice = 0;

				// Handle for heap start
				auto handle{ context.m_viewHeap->GetCPUDescriptorHandleForHeapStart() };
				handle.ptr += offset * context.m_viewHeapIncrement;

				// Creates view
				device->CreateUnorderedAccessView(rwResources.m_HI_Z.pyramid.Get(), nullptr, &uavDesc, handle);

				// Increments offset 
				offset++;
			}
		}

	}

	void RecreateDepthTargetDescriptor(ID3D12Device* device, DX12WRAPPER<ID3D12Resource>* pDepthTargets, DescriptorContext& ctx)
	{
		for (size_t i = 0; i < ce_framesInFlight; ++i)
		{
			SIZE_T offset{ ctx.m_depthTargetOffset[i] };

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = Ce_DepthTargetSRVFormat;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.PlaneSlice = 0;

			// handle for heap start
			auto handle{ ctx.m_viewHeap->GetCPUDescriptorHandleForHeapStart() };
			handle.ptr += offset * ctx.m_viewHeapIncrement;

			// Creates view
			device->CreateShaderResourceView(pDepthTargets[i].Get(), &srvDesc, handle);

			offset++;
		}
	}
}

#endif