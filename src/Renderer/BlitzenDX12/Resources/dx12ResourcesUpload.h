#if defined(_WIN32)

#pragma once
#include "Renderer/BlitzenDX12/Context/dx12Context.h"
#include "Renderer/Resources/Textures/blitTextures.h"
#include "Renderer/Interface/blitRendererInterface.h"

namespace BlitzenDX12
{
	DXGI_FORMAT GetDDSFormat(const BlitzenEngine::DDS_HEADER& header, const BlitzenEngine::DDS_HEADER_DXT10& header10);

	uint8_t Create2DTexture(ID3D12Device* device, DX12WRAPPER<ID3D12Resource>& resource, UINT width, UINT height, UINT mipLevels,
		DXGI_FORMAT format, UINT blockSize, CmdContext& cmdContext, ID3D12CommandQueue* commandQueue, DX12WRAPPER<ID3D12Resource>& staging);

	uint8_t UploadResourcesToBuffers(ID3D12Device* device, const BlitzenEngine::DrawContext& drawContext, ReadOnlyResources& roResources, ReadWriteResources* rwResourcesArr,
		CmdContext& cmdContext, ID3D12CommandQueue* commandQueue);

	void CreateResourceViews(ID3D12Device* device, DescriptorContext& ctx, CmdContext& cmdContext, ID3D12CommandQueue* queue, ReadOnlyResources& roResources,
		ReadWriteResources* rwResourcesArray, BlitzenEngine::DrawContext& context, DX12WRAPPER<ID3D12Resource>* pDepthTargets, UINT drawWidth, UINT drawHeight);
}

#endif