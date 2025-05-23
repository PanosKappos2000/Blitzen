#include "dx12Renderer.h"
#include "dx12Pipelines.h"
#include "dx12Resources.h"
#include "dx12RNDResources.h"

namespace BlitzenDX12
{
	static DXGI_FORMAT GetDDSFormat(const BlitzenEngine::DDS_HEADER& header, const BlitzenEngine::DDS_HEADER_DXT10& header10)
	{
		if (header.ddspf.dwFourCC == BlitzenEngine::FourCC("DXT1"))
		{
			//return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
			return DXGI_FORMAT_BC1_UNORM;
			
		}
		if (header.ddspf.dwFourCC == BlitzenEngine::FourCC("DXT3"))
		{
			//return VK_FORMAT_BC2_UNORM_BLOCK;
			return DXGI_FORMAT_BC2_UNORM;
		}
		if (header.ddspf.dwFourCC == BlitzenEngine::FourCC("DXT5"))
		{
			//return VK_FORMAT_BC3_UNORM_BLOCK;
			return DXGI_FORMAT_BC3_UNORM;
		}

		if (header.ddspf.dwFourCC == BlitzenEngine::FourCC("DX10"))
		{
			switch (header10.dxgiFormat)
			{
			case BlitzenEngine::DXGI_FORMAT_BC1_UNORM:
			case BlitzenEngine::DXGI_FORMAT_BC1_UNORM_SRGB:
			case BlitzenEngine::DXGI_FORMAT_BC2_UNORM:
			case BlitzenEngine::DXGI_FORMAT_BC2_UNORM_SRGB:
			case BlitzenEngine::DXGI_FORMAT_BC3_UNORM:
			case BlitzenEngine::DXGI_FORMAT_BC3_UNORM_SRGB:
			case BlitzenEngine::DXGI_FORMAT_BC4_UNORM:
			case BlitzenEngine::DXGI_FORMAT_BC4_SNORM:
			case BlitzenEngine::DXGI_FORMAT_BC5_UNORM:
			case BlitzenEngine::DXGI_FORMAT_BC5_SNORM:
			case BlitzenEngine::DXGI_FORMAT_BC6H_UF16:
			case BlitzenEngine::DXGI_FORMAT_BC6H_SF16:
			case BlitzenEngine::DXGI_FORMAT_BC7_UNORM:
			case BlitzenEngine::DXGI_FORMAT_BC7_UNORM_SRGB:
				return (DXGI_FORMAT)header10.dxgiFormat;
			}
		}

		return DXGI_FORMAT_UNKNOWN;
	}

	static uint8_t LoadDDSImageData(BlitzenEngine::DDS_HEADER& header, BlitzenEngine::DDS_HEADER_DXT10& header10, BlitzenPlatform::FileHandle& handle, 
		DXGI_FORMAT& format, void* pData, uint32_t& blockSize)
	{
		format = GetDDSFormat(header, header10);
		if (format == DXGI_FORMAT_UNKNOWN)
		{
			return 0;
		}

		auto file = reinterpret_cast<FILE*>(handle.pHandle);
		blockSize = BlitzenEngine::GetDDSBlockSize(header, header10);
		auto imageSize = BlitzenEngine::GetDDSImageSizeBC(header.dwWidth, header.dwHeight, header.dwMipMapCount, blockSize);
		auto readSize = fread(pData, 1, imageSize, file);
		if (!pData)
		{
			BLIT_ERROR("Failed to read texture data");
			return 0;
		}
		if (readSize != imageSize)
		{
			BLIT_ERROR("Failed to read the correct amount of texture data. Expected: %u, Read: %u", imageSize, readSize);
			return 0;
		}

		// Success
		return 1;
	}

	static uint8_t Create2DTexture(ID3D12Device* device, DX12WRAPPER<ID3D12Resource>& resource, UINT width, UINT height, UINT mipLevels,
		DXGI_FORMAT format, UINT blockSize, Dx12Renderer::FrameTools& tools, ID3D12CommandQueue* commandQueue, DX12WRAPPER<ID3D12Resource>& staging)
	{
		if (!CreateImageResource(device, resource.ReleaseAndGetAddressOf(), width, height, mipLevels, format, D3D12_RESOURCE_FLAG_NONE,
			D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, nullptr, D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE))
		{
			BLIT_ERROR("Failed to create texture image resource");
			return 0;
		}

		UINT bufferOffset{ 0 };
		UINT mipWidth{ width };
		UINT mipHeight{ height };

		tools.transferCommandAllocator->Reset();
		tools.transferCommandList->Reset(tools.transferCommandAllocator.Get(), nullptr);

		// Transition texture to COPY_DEST state
		D3D12_RESOURCE_BARRIER preCopyBarriers[2]{};
		CreateResourcesTransitionBarrier(preCopyBarriers[0], resource.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		CreateResourcesTransitionBarrier(preCopyBarriers[1], staging.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		tools.transferCommandList->ResourceBarrier(BLIT_ARRAY_SIZE(preCopyBarriers), preCopyBarriers);

		// Create copy regions for each mip level
		for (UINT i = 0; i < mipLevels; ++i)
		{
			D3D12_TEXTURE_COPY_LOCATION dst{};
			dst.pResource = resource.Get();
			dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dst.SubresourceIndex = i;

			D3D12_TEXTURE_COPY_LOCATION src = {};
			src.pResource = staging.Get();
			src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			src.PlacedFootprint.Offset = bufferOffset;
			src.PlacedFootprint.Footprint.Format = format;
			src.PlacedFootprint.Footprint.Width = mipWidth;
			src.PlacedFootprint.Footprint.Height = mipHeight;
			src.PlacedFootprint.Footprint.Depth = 1;
			src.PlacedFootprint.Footprint.RowPitch = ((mipWidth + 3) / 4) * blockSize;

			// Define the copy region (size of the mip level)
			D3D12_BOX box{};
			box.left = 0;
			box.top = 0;
			box.front = 0;
			box.right = (mipWidth + 3) / 4 * 4;  // Round up to 4-byte alignment
			box.bottom = (mipHeight + 3) / 4 * 4;  // Same for height
			box.back = 1;

			tools.transferCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, &box);

			bufferOffset += ((mipWidth + 3) / 4) * ((mipHeight + 3) / 4) * blockSize;
			mipWidth = mipWidth > 1 ? mipWidth / 2 : 1;
			mipHeight = mipHeight > 1 ? mipHeight / 2 : 1;
		}

		tools.transferCommandList->Close();
		ID3D12CommandList* commandLists[] = { tools.transferCommandList.Get() };
		commandQueue->ExecuteCommandLists(1, commandLists);

		PlaceFence(tools.copyFenceValue, commandQueue, tools.copyFence.Get(), tools.copyFenceEvent);

		return 1;
	}

	uint8_t Dx12Renderer::UploadTexture(const char* filepath)
	{
		// DDS data Loading
		BlitzenEngine::DDS_HEADER header{};
		BlitzenEngine::DDS_HEADER_DXT10 header10{};
		BlitzenPlatform::FileHandle handle{};
		if (!BlitzenEngine::OpenDDSImageFile(filepath, header, header10, handle))
		{
			BLIT_ERROR("Failed to open texture file");
			return 0;
		}

		// Staging buffer to hold the data
		DX12WRAPPER<ID3D12Resource> stagingBuffer;
		if (!CreateBuffer(m_device.Get(), stagingBuffer.ReleaseAndGetAddressOf(), Ce_TextureDataStagingSize, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_UPLOAD))
		{
			BLIT_ERROR("Failed to create staging buffer for texture data copy");
			return 0;
		}

		void* pData{ nullptr };
		auto mappingRes = stagingBuffer->Map(0, nullptr, &pData);
		if (FAILED(mappingRes))
		{
			return LOG_ERROR_MESSAGE_AND_RETURN(mappingRes);
		}

		// Loads the texture data
		auto& tex2D{ m_tex2DList[m_textureCount] };
		uint32_t blockSize{ 0 };
		if (!LoadDDSImageData(header, header10, handle, tex2D.format, pData, blockSize))
		{
			BLIT_ERROR("Failed to load texture data");
			return 0;
		}

		tex2D.mipLevels = header.dwMipMapCount;
		if (!Create2DTexture(m_device.Get(), tex2D.resource, header.dwWidth, header.dwHeight, tex2D.mipLevels, tex2D.format, 
			blockSize, m_frameTools[m_currentFrame], m_transferCommandQueue.Get(), stagingBuffer))
		{
			BLIT_ERROR("Failed to load texture to dx12");
			return 0;
		}

		stagingBuffer->Unmap(0, nullptr);

		m_textureCount++;

		return 1;
	}

	static uint8_t CreateTestIndirectCmds(ID3D12Device* device, Dx12Renderer::FrameTools& frameTools, ID3D12CommandQueue* commandQueue,
		Dx12Renderer::VarBuffers& buffers, BlitzenEngine::DrawContext& context)
	{
		const uint32_t maxCmdCount = 1000;
		auto pRenders{ context.m_renders.m_renders };
		uint32_t cmdCount = maxCmdCount;
		auto renderCount{ context.m_renders.m_renderCount };
		if (cmdCount > renderCount)
		{
			cmdCount = renderCount;
		}

		const auto& surfaces{ context.m_meshes.m_surfaces };
		const auto& lods{ context.m_meshes.m_LODs };
		BlitCL::StaticArray<IndirectDrawCmd, maxCmdCount> cmds;

		for (uint32_t i = 0; i < cmdCount; ++i)
		{
			auto& cmd = cmds[i];
			auto& surface = surfaces[pRenders[i].surfaceId];
			auto& lod = lods[surface.lodOffset];

			cmd.objId = i;
			cmd.command.IndexCountPerInstance = lod.indexCount;
			cmd.command.StartIndexLocation = lod.firstIndex;
			cmd.command.BaseVertexLocation = 0;
			cmd.command.InstanceCount = 1;
			cmd.command.StartInstanceLocation = 0;
		}

		DX12WRAPPER<ID3D12Resource> cmdStagingBuffer{ nullptr };
		if (!CreateBuffer(device, cmdStagingBuffer.ReleaseAndGetAddressOf(), cmdCount * sizeof(IndirectDrawCmd), D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_UPLOAD))
		{
			BLIT_ERROR("Failed test function CreateTestIndirectCmds");
			return 0;
		}

		DX12WRAPPER<ID3D12Resource> countStagingBuffer{ nullptr };
		if (!CreateBuffer(device, countStagingBuffer.ReleaseAndGetAddressOf(), sizeof(uint32_t), D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_UPLOAD))
		{
			BLIT_ERROR("Failed test function CreateTestIndirectCmds");
			return 0;
		}

		void* pMappedCmd{ nullptr };
		auto mappingRes{ cmdStagingBuffer->Map(0, nullptr, &pMappedCmd) };
		if (FAILED(mappingRes))
		{
			return LOG_ERROR_MESSAGE_AND_RETURN(mappingRes);
		}
		BlitzenCore::BlitMemCopy(pMappedCmd, cmds.Data(), sizeof(IndirectDrawCmd) * cmdCount);

		void* pMappedCount{ nullptr };
		mappingRes = cmdStagingBuffer->Map(0, nullptr, &pMappedCount);
		if (FAILED(mappingRes))
		{
			return LOG_ERROR_MESSAGE_AND_RETURN(mappingRes);
		}
		BlitzenCore::BlitMemCopy(pMappedCount, &cmdCount, sizeof(uint32_t));

		frameTools.transferCommandAllocator->Reset();
		frameTools.transferCommandList->Reset(frameTools.transferCommandAllocator.Get(), nullptr);

		// Transitions SSBOs to copy dest
		D3D12_RESOURCE_BARRIER copyDestBarriers[2]{};
		CreateResourcesTransitionBarrier(copyDestBarriers[0], buffers.indirectDrawBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		CreateResourcesTransitionBarrier(copyDestBarriers[1], buffers.indirectDrawCount.buffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		frameTools.transferCommandList->ResourceBarrier(2, copyDestBarriers);

		// Transitions stagingBuffers to copy source
		D3D12_RESOURCE_BARRIER copySourceBarriers[2]{};
		CreateResourcesTransitionBarrier(copySourceBarriers[0], cmdStagingBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		CreateResourcesTransitionBarrier(copySourceBarriers[1], countStagingBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		frameTools.transferCommandList->ResourceBarrier(2, copySourceBarriers);

		frameTools.transferCommandList->CopyResource(buffers.indirectDrawBuffer.buffer.Get(), cmdStagingBuffer.Get());
		frameTools.transferCommandList->CopyResource(buffers.indirectDrawCount.buffer.Get(), countStagingBuffer.Get());

		// Switches back to common, because it will be expected to be in that state later
		D3D12_RESOURCE_BARRIER switchBack[2]{};
		CreateResourcesTransitionBarrier(switchBack[0], buffers.indirectDrawBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
		CreateResourcesTransitionBarrier(switchBack[1], buffers.indirectDrawCount.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
		frameTools.transferCommandList->ResourceBarrier(2, switchBack);

		frameTools.transferCommandList->Close();
		ID3D12CommandList* commandLists[] = { frameTools.transferCommandList.Get() };
		commandQueue->ExecuteCommandLists(1, commandLists);

		const UINT64 fence = frameTools.copyFenceValue++;
		commandQueue->Signal(frameTools.copyFence.Get(), fence);
		// Waits for the previous frame
		if (frameTools.copyFence->GetCompletedValue() < fence)
		{
			frameTools.copyFence->SetEventOnCompletion(fence, frameTools.copyFenceEvent);
			WaitForSingleObject(frameTools.copyFenceEvent, INFINITE);
		}

		return 1;
	}

	static uint8_t CreateRootSignatures(ID3D12Device* device, ID3D12RootSignature** ppOpaqueRootSignature, 
		ID3D12RootSignature** ppCullRootSignature, ID3D12RootSignature** ppResetSignature, ID3D12RootSignature** ppDrawOccSignature, 
		ID3D12RootSignature** ppDepthPyramidSignature)
	{
		D3D12_DESCRIPTOR_RANGE opaqueSrvRanges[Ce_OpaqueSrvRangeCount]{};
		CreateDescriptorRange(opaqueSrvRanges[Ce_VertexBufferRangeElement], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Ce_VertexBufferDescriptorCount, Ce_VertexBufferRegister);

		BlitCL::DynamicArray<D3D12_DESCRIPTOR_RANGE> sharedSrvRanges{ Ce_SharedSrvRangeCount };
		CreateDescriptorRange(sharedSrvRanges[Ce_SurfaceBufferRangeElement], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Ce_SurfaceBufferDescriptorCount, Ce_SurfaceBufferRegister);
		CreateDescriptorRange(sharedSrvRanges[Ce_TransformBufferRangeElement], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Ce_TransformBufferDescriptorCount, Ce_TransformBufferRegister);
		CreateDescriptorRange(sharedSrvRanges[Ce_RenderObjectBufferRangeElement], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Ce_RenderObjectBufferDescriptorCount, Ce_RenderObjectBufferRegister);
		CreateDescriptorRange(sharedSrvRanges[Ce_ViewDataBufferRangeElement], D3D12_DESCRIPTOR_RANGE_TYPE_CBV, Ce_ViewDataBufferDescriptorCount, Ce_ViewDataBufferRegister);

		// ADDITIONAL
		if constexpr (BlitzenCore::Ce_InstanceCulling && !CE_DX12OCCLUSION)// draw occlusion mode does not have instancing
		{
			D3D12_DESCRIPTOR_RANGE instanceBufferRange{};
			CreateDescriptorRange(instanceBufferRange, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, Ce_InstanceBufferDescriptorCount, Ce_InstanceBufferRegister);
			sharedSrvRanges.PushBack(instanceBufferRange);
		}

		D3D12_DESCRIPTOR_RANGE textureSamplerRange{};
		CreateDescriptorRange(textureSamplerRange, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, Ce_DefaultTextureSamplerDescriptorCount, Ce_DefaultTextureSamplerRegister);

		D3D12_DESCRIPTOR_RANGE materialSrvRange{};
		CreateDescriptorRange(materialSrvRange, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Ce_MaterialBufferDescriptorCount, Ce_MaterialBufferRegister);

		D3D12_DESCRIPTOR_RANGE textureSrvsRange{};
		CreateDescriptorRange(textureSrvsRange, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Ce_TextureDescriptorCount, Ce_TextureDescriptorsRegister);

		D3D12_ROOT_PARAMETER rootParameters[Ce_OpaqueRootParameterCount]{};
		CreateRootParameterDescriptorTable(rootParameters[Ce_OpaqueExclusiveBuffersElement], opaqueSrvRanges, Ce_OpaqueSrvRangeCount, D3D12_SHADER_VISIBILITY_VERTEX);
		CreateRootParameterDescriptorTable(rootParameters[Ce_OpaqueSharedBuffersElement], sharedSrvRanges.Data(), (UINT)sharedSrvRanges.GetSize(), D3D12_SHADER_VISIBILITY_VERTEX);
		CreateRootParameterPushConstants(rootParameters[Ce_OpaqueObjectIdElement], Ce_ObjectIdRegister, 0, 1, D3D12_SHADER_VISIBILITY_VERTEX);
		CreateRootParameterDescriptorTable(rootParameters[Ce_OpaqueSamplerElement], &textureSamplerRange, 1, D3D12_SHADER_VISIBILITY_PIXEL);
		CreateRootParameterDescriptorTable(rootParameters[Ce_MaterialSrvElement], &materialSrvRange, 1, D3D12_SHADER_VISIBILITY_PIXEL);
		CreateRootParameterDescriptorTable(rootParameters[Ce_TextureDescriptorsElement], &textureSrvsRange, 1, D3D12_SHADER_VISIBILITY_PIXEL);

		if (!CreateRootSignature(device, ppOpaqueRootSignature, Ce_OpaqueRootParameterCount, rootParameters, D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED))
		{
			BLIT_ERROR("Failed to create opaque root signature");
			return 0;
		}

		BlitCL::DynamicArray<D3D12_DESCRIPTOR_RANGE> cullSrvRanges{ Ce_CullSrvRangeCount };
		CreateDescriptorRange(cullSrvRanges[Ce_IndirectDrawBufferRangeElement], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, Ce_IndirectDrawBufferDescriptorCount, Ce_IndirectDrawBufferRegister);
		CreateDescriptorRange(cullSrvRanges[Ce_IndirectCountBufferRangeElement], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, Ce_IndirectCountBufferDescriptorCount, Ce_IndirectCountBufferRegister);
		CreateDescriptorRange(cullSrvRanges[Ce_LODBufferRangeElement], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Ce_LODBufferDescriptorCount, Ce_LODBufferRegister);
		if constexpr (CE_DX12OCCLUSION)
		{
			D3D12_DESCRIPTOR_RANGE drawVisibilityBufferRange{};
			CreateDescriptorRange(drawVisibilityBufferRange, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, Ce_DrawVisibilityBufferDescriptorCount, Ce_DrawVisibilityBufferRegister);
			cullSrvRanges.PushBack(drawVisibilityBufferRange);
		}
		else if constexpr (BlitzenCore::Ce_InstanceCulling)
		{
			D3D12_DESCRIPTOR_RANGE lodInstanceBufferRange{};
			CreateDescriptorRange(lodInstanceBufferRange, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, Ce_LODInstanceBufferDescriptorCount, Ce_LODInstanceBufferRegister);
			cullSrvRanges.PushBack(lodInstanceBufferRange);
		}

		D3D12_ROOT_PARAMETER drawCullRootParameters[Ce_CullRootParameterCount]{};
		CreateRootParameterDescriptorTable(drawCullRootParameters[Ce_CullExclusiveSRVsParameterId], cullSrvRanges.Data(), (UINT)cullSrvRanges.GetSize(), D3D12_SHADER_VISIBILITY_ALL);
		CreateRootParameterDescriptorTable(drawCullRootParameters[Ce_CullSharedSRVsParameterId], sharedSrvRanges.Data(), (UINT)sharedSrvRanges.GetSize(), D3D12_SHADER_VISIBILITY_ALL);
		CreateRootParameterPushConstants(drawCullRootParameters[Ce_CullDrawCountParameterId], Ce_CullShaderRootConstantRegister, 0, 1, D3D12_SHADER_VISIBILITY_ALL);

		if (!CreateRootSignature(device, ppCullRootSignature, Ce_CullRootParameterCount, drawCullRootParameters))
		{
			BLIT_ERROR("Failed to create draw cull root signature");
			return 0;
		}

		D3D12_ROOT_PARAMETER resetShaderRootParameter{};
		CreateRootParameterDescriptorTable(resetShaderRootParameter, cullSrvRanges.Data(), (UINT)cullSrvRanges.GetSize(), D3D12_SHADER_VISIBILITY_ALL);
		if (!CreateRootSignature(device, ppResetSignature, 1, &resetShaderRootParameter))
		{
			BLIT_ERROR("Failed to create draw count reset shader push constant");
			return 0;
		}

		if (CE_DX12OCCLUSION)
		{
			D3D12_DESCRIPTOR_RANGE depthPyramidCullRange{};
			CreateDescriptorRange(depthPyramidCullRange, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Ce_DepthPyramidCullDescriptorCount, Ce_DepthPyramidCullRegister);

			D3D12_ROOT_PARAMETER drawOccLateRootParameters[Ce_DrawOccLateRootParameterCount]{};
			CreateRootParameterDescriptorTable(drawOccLateRootParameters[Ce_CullExclusiveSRVsParameterId], cullSrvRanges.Data(), (UINT)cullSrvRanges.GetSize(), D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(drawOccLateRootParameters[Ce_CullSharedSRVsParameterId], sharedSrvRanges.Data(), (UINT)sharedSrvRanges.GetSize(), D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterPushConstants(drawOccLateRootParameters[Ce_CullDrawCountParameterId], Ce_CullShaderRootConstantRegister, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(drawOccLateRootParameters[Ce_DrawOccLateDepthPyramidParameterId], &depthPyramidCullRange, 1, D3D12_SHADER_VISIBILITY_ALL);

			if (!CreateRootSignature(device, ppDrawOccSignature, Ce_DrawOccLateRootParameterCount, drawOccLateRootParameters))
			{
				BLIT_ERROR("Failed to create late cull (occlusion culling) root parameter");
				return 0;
			}

			D3D12_DESCRIPTOR_RANGE depthPyramidUAVRange{};
			CreateDescriptorRange(depthPyramidUAVRange, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, Ce_DepthPyramidGenUAVDescriptorCount, Ce_DepthPyramidGenUAVRegister);

			D3D12_DESCRIPTOR_RANGE depthPyramidSRVRange{};
			CreateDescriptorRange(depthPyramidSRVRange, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Ce_DepthPyramidSRVDescriptorCount, Ce_DepthPyramidSRVRegister);

			D3D12_ROOT_PARAMETER depthPyramidGenParameters[Ce_DepthPyramidParameterCount]{};
			CreateRootParameterDescriptorTable(depthPyramidGenParameters[Ce_DepthPyramidUAVRootParameterId], &depthPyramidUAVRange, 1, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(depthPyramidGenParameters[Ce_DepthPyramidSRVRootParameterId], &depthPyramidSRVRange, 1, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterPushConstants(depthPyramidGenParameters[Ce_DepthPyramidRootConstantParameterId], Ce_DepthPyramidRootConstantsRegister, 
				0, Ce_DepthPyramidRootConstantsCount, D3D12_SHADER_VISIBILITY_ALL);

			if (!CreateRootSignature(device, ppDepthPyramidSignature, Ce_DepthPyramidParameterCount, depthPyramidGenParameters))
			{
				BLIT_ERROR("Failed to create depth pyramid root parameter");
				return 0;
			}
		}


		// success
		return 1;
	}

	static uint8_t CreateCmdSignatures(ID3D12Device* device, ID3D12RootSignature* opaqueRootSignature, ID3D12CommandSignature** ppOpaqueCmdSignature)
	{
		// Draw command
		D3D12_INDIRECT_ARGUMENT_DESC indirectDescs[2]{};
		indirectDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

		// Object id root constant
		indirectDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
		indirectDescs[0].Constant.DestOffsetIn32BitValues = 0;
		indirectDescs[0].Constant.Num32BitValuesToSet = 1;
		indirectDescs[0].Constant.RootParameterIndex = Ce_OpaqueObjectIdElement;

		D3D12_COMMAND_SIGNATURE_DESC sigDesc{};
		sigDesc.NodeMask = 0;
		sigDesc.NumArgumentDescs = 2;
		sigDesc.pArgumentDescs = indirectDescs;
		sigDesc.ByteStride = sizeof(IndirectDrawCmd);
		auto opaqueCmdRes{ device->CreateCommandSignature(&sigDesc, opaqueRootSignature, IID_PPV_ARGS(ppOpaqueCmdSignature)) };
		if (FAILED(opaqueCmdRes))
		{
			return LOG_ERROR_MESSAGE_AND_RETURN(opaqueCmdRes);
		}

		// success
		return 1;
	}

	struct PipelineCreationContext
	{
		ID3D12RootSignature* opaqueRoot; 
		ID3D12PipelineState** ppOpaquePso;

		ID3D12RootSignature* drawCullRoot;
		ID3D12PipelineState** ppDrawCullPso;
		ID3D12PipelineState** ppDrawInstCmdPso;
		ID3D12PipelineState** ppDrawInstCountResetPso;

		ID3D12RootSignature* resetRoot;
		ID3D12PipelineState** ppResetPso;

		ID3D12RootSignature* drawOccRoot;
		ID3D12PipelineState** ppDrawOccPso;

		ID3D12RootSignature* depthPyramidRoot;
		ID3D12PipelineState** ppDepthPyramidPso;
	};
	static uint8_t CreatePipelines(ID3D12Device* device, PipelineCreationContext& context)
	{
		if (!CreateOpaqueGraphicsPipeline(device, context.opaqueRoot, context.ppOpaquePso))
		{
			BLIT_ERROR("Failed to create opaque grahics pipeline");
			return 0;
		}

		if constexpr (CE_DX12OCCLUSION)
		{
			if (!CreateComputeShaderProgram(device, context.drawCullRoot, context.ppDrawCullPso, "HlslShaders/CS/drawOccFirst.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create drawCull.cs shader program");
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.drawOccRoot, context.ppDrawOccPso, "HlslShaders/CS/drawOccLate.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create drawOccLate.cs shader program");
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.depthPyramidRoot, context.ppDepthPyramidPso, "HlslShaders/CS/depthPyramid.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create depthPyramid.cs shader program");
				return 0;
			}
		}
		else if constexpr (BlitzenCore::Ce_InstanceCulling)
		{
			if (!CreateComputeShaderProgram(device, context.drawCullRoot, context.ppDrawCullPso, "HlslShaders/CS/drawInstCull.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create drawInstCull.cs shader program");
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.drawCullRoot, context.ppDrawInstCmdPso, "HlslShaders/CS/drawInstCmd.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create drawInstCull.cs shader program");
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.drawCullRoot, context.ppDrawInstCountResetPso, "HlslShaders/CS/drawInstCountReset.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create drawInstCountReset.cs shader program");
				return 0;
			}
		}
		else
		{
			if (!CreateComputeShaderProgram(device, context.drawCullRoot, context.ppDrawCullPso, "HlslShaders/CS/drawCull.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create drawCull.cs shader program");
				return 0;
			}
		}

		if (!CreateComputeShaderProgram(device, context.resetRoot, context.ppResetPso, "HlslShaders/CS/drawCountReset.cs.hlsl.bin"))
		{
			BLIT_ERROR("Failed to create drawCountReset.cs shader program");
			return 0;
		}

		return 1;
	}

	static uint8_t CopyDataToVarSSBOs(ID3D12Device* device, Dx12Renderer::FrameTools& frameTools, Dx12Renderer::VarBuffers& buffers,
		ID3D12CommandQueue* commandQueue)
	{
		frameTools.transferCommandAllocator->Reset();
		frameTools.transferCommandList->Reset(frameTools.transferCommandAllocator.Get(), nullptr);

		// Transitions SSBOs to copy dest
		D3D12_RESOURCE_BARRIER copyDestBarriers[Ce_VarSSBODataCount]{};
		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_TransformStagingBufferIndex], buffers.transformBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		frameTools.transferCommandList->ResourceBarrier(Ce_VarSSBODataCount, copyDestBarriers);

		// Transitions stagingBuffers to copy source
		D3D12_RESOURCE_BARRIER copySourceBarriers[Ce_VarSSBODataCount]{};
		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_TransformStagingBufferIndex], buffers.transformBuffer.staging.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		frameTools.transferCommandList->ResourceBarrier(BLIT_ARRAY_SIZE(copySourceBarriers), copySourceBarriers);

		frameTools.transferCommandList->CopyResource(buffers.transformBuffer.buffer.Get(), buffers.transformBuffer.staging.Get());

		frameTools.transferCommandList->Close();
		ID3D12CommandList* commandLists[] = { frameTools.transferCommandList.Get() };
		commandQueue->ExecuteCommandLists(1, commandLists);

		const UINT64 fence = frameTools.copyFenceValue++;
		commandQueue->Signal(frameTools.copyFence.Get(), fence);
		// Waits for the previous frame
		if (frameTools.copyFence->GetCompletedValue() < fence)
		{
			frameTools.copyFence->SetEventOnCompletion(fence, frameTools.copyFenceEvent);
			WaitForSingleObject(frameTools.copyFenceEvent, INFINITE);
		}

		return 1;
	}

	static uint8_t CreateVarBuffers(ID3D12Device* device, ID3D12CommandQueue* commandQueue, Dx12Renderer::FrameTools& frameTools, 
		Dx12Renderer::VarBuffers* varBuffers, BlitzenEngine::DrawContext& context, uint32_t swapchainWidth, uint32_t swapchainHeight)
	{
		const auto& transforms{ context.m_renders.m_transforms };
		const auto& lodData{ context.m_meshes.m_LODs };
		const auto& lodInstanceList{ context.m_meshes.m_lodInstanceList };
		auto renderCount{ context.m_renders.m_renderCount };
		auto dynamicTransformCount{ context.m_renders.m_dynamicTransformCount };

		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			auto& buffers = varBuffers[i];

			if (!CreateCBuffer(device, buffers.viewDataBuffer))
			{
				BLIT_ERROR("Failed to create view data buffer");
				return 0;
			}

			DX12WRAPPER<ID3D12Resource> transformStaging;
			if (!CreateVarSSBO(device, buffers.transformBuffer, transformStaging, context.m_renders.m_transformCount, context.m_renders.m_transforms, dynamicTransformCount))
			{
				BLIT_ERROR("Failed to create transform buffer");
				return 0;
			}

			UINT64 indirectBufferSize{ Ce_IndirectDrawCmdBufferSize * sizeof(IndirectDrawCmd) };
			if (!CreateBuffer(device, buffers.indirectDrawBuffer.buffer.ReleaseAndGetAddressOf(), indirectBufferSize,
				D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
			{
				BLIT_ERROR("Failed to create indirect draw buffer");
				return 0;
			}

			if (!CreateBuffer(device, buffers.indirectDrawCount.buffer.ReleaseAndGetAddressOf(), sizeof(uint32_t),
				D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
			{
				BLIT_ERROR("Failed to create indirect count buffer");
				return 0;
			}


			// Draw Instance mode buffer data( kept in scope)
			DX12WRAPPER<ID3D12Resource> lodInstStaging{ nullptr };
			UINT64 lodInstanceBufferSize{ 0 };

			DX12WRAPPER<ID3D12Resource> drawVisibilityStaging{ nullptr };
			UINT64 visibilityBufferSize{ 0 };

			if constexpr (CE_DX12OCCLUSION)
			{
				BlitCL::DynamicArray<uint32_t> zeroData{ renderCount, 0 };
				
				visibilityBufferSize = CreateSSBO(device, buffers.drawVisibilityBuffer, drawVisibilityStaging, (size_t)renderCount,
					zeroData.Data(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
				if(!visibilityBufferSize)
				{
					BLIT_ERROR("Failed to create draw visibility buffer for draw occlusion");
					return 0;
				}

				if (!CreateDepthPyramidResource(device, buffers.depthPyramid, swapchainWidth, swapchainHeight))
				{
					BLIT_ERROR("Failed to create depth pyramid for occlusion culling");
					return 0;
				}
			}
			else if constexpr (BlitzenCore::Ce_InstanceCulling)
			{
				UINT64 instanceBufferSize{ lodData.GetSize() * BlitzenCore::Ce_MaxInstanceCountPerLOD * sizeof(uint32_t) };
				if (!CreateBuffer(device, buffers.drawInstBuffer.buffer.ReleaseAndGetAddressOf(), instanceBufferSize,
					D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
				{
					BLIT_ERROR("Failed to create instance buffer");
					return 0;
				}

				lodInstanceBufferSize = CreateSSBO(device, buffers.lodInstBuffer, lodInstStaging, lodInstanceList.GetSize(), lodInstanceList.Data(),
					D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
				if (!lodInstanceBufferSize)
				{
					BLIT_ERROR("Failed to create lod instance counter buffer");
					return 0;
				}
			}

			frameTools.transferCommandAllocator->Reset();
			frameTools.transferCommandList->Reset(frameTools.transferCommandAllocator.Get(), nullptr);

			BlitCL::DynamicArray<D3D12_RESOURCE_BARRIER> copyDestBarriers{ Ce_VarSSBODataCount };
			CreateResourcesTransitionBarrier(copyDestBarriers[0], buffers.transformBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

			// Conditional barriers
			if constexpr (CE_DX12OCCLUSION)
			{
				D3D12_RESOURCE_BARRIER visibilityBufferDestBarrier{};
				CreateResourcesTransitionBarrier(visibilityBufferDestBarrier, buffers.drawVisibilityBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
				copyDestBarriers.PushBack(visibilityBufferDestBarrier);
			}
			else if constexpr (BlitzenCore::Ce_InstanceCulling)
			{
				D3D12_RESOURCE_BARRIER lodInstBufferDestBarrier{};
				CreateResourcesTransitionBarrier(lodInstBufferDestBarrier, buffers.lodInstBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
				copyDestBarriers.PushBack(lodInstBufferDestBarrier);
			}

			// Execute
			frameTools.transferCommandList->ResourceBarrier((UINT)copyDestBarriers.GetSize(), copyDestBarriers.Data());

			// Staging barriers
			BlitCL::DynamicArray<D3D12_RESOURCE_BARRIER> copySourceBarriers{ Ce_VarSSBODataCount };
			CreateResourcesTransitionBarrier(copySourceBarriers[0], transformStaging.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

			// Conditional staging barriers
			if constexpr (CE_DX12OCCLUSION)
			{
				D3D12_RESOURCE_BARRIER visibilityBufferSourceBarrier{};
				CreateResourcesTransitionBarrier(visibilityBufferSourceBarrier, drawVisibilityStaging.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
				copySourceBarriers.PushBack(visibilityBufferSourceBarrier);
			}
			else if constexpr (BlitzenCore::Ce_InstanceCulling)
			{
				D3D12_RESOURCE_BARRIER lodInstBufferSourceBarrier{};
				CreateResourcesTransitionBarrier(lodInstBufferSourceBarrier, lodInstStaging.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
				copySourceBarriers.PushBack(lodInstBufferSourceBarrier);
			}

			// Execute
			frameTools.transferCommandList->ResourceBarrier(UINT(copySourceBarriers.GetSize()), copySourceBarriers.Data());

			// Copy
			frameTools.transferCommandList->CopyResource(buffers.transformBuffer.buffer.Get(), transformStaging.Get());

			// Conditional copy
			if constexpr (CE_DX12OCCLUSION)
			{
				frameTools.transferCommandList->CopyResource(buffers.drawVisibilityBuffer.buffer.Get(), drawVisibilityStaging.Get());
			}
			else if constexpr (BlitzenCore::Ce_InstanceCulling)
			{
				frameTools.transferCommandList->CopyResource(buffers.lodInstBuffer.buffer.Get(), lodInstStaging.Get());
			}

			D3D12_RESOURCE_BARRIER dynamicTransformBarrier{};
			CreateResourcesTransitionBarrier(dynamicTransformBarrier, buffers.transformBuffer.staging.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
			frameTools.transferCommandList->ResourceBarrier(1, &dynamicTransformBarrier);

			frameTools.transferCommandList->Close();
			ID3D12CommandList* commandLists[] = { frameTools.transferCommandList.Get() };
			commandQueue->ExecuteCommandLists(1, commandLists);

			const UINT64 fence = frameTools.copyFenceValue++;
			commandQueue->Signal(frameTools.copyFence.Get(), fence);
			// Waits for the previous frame
			if (frameTools.copyFence->GetCompletedValue() < fence)
			{
				frameTools.copyFence->SetEventOnCompletion(fence, frameTools.copyFenceEvent);
				WaitForSingleObject(frameTools.copyFenceEvent, INFINITE);
			}
		}
		// Success
		return 1;
	}

	struct SSBOCopyContext
	{
		ID3D12Resource* stagingBuffers[Ce_ConstDataSSBOCount];
		UINT64 stagingBufferSizes[Ce_ConstDataSSBOCount];
	};
	static uint8_t CopyDataToSSBOs(ID3D12Device* device, Dx12Renderer::FrameTools& frameTools, Dx12Renderer::ConstBuffers& buffers, 
		SSBOCopyContext& context, ID3D12CommandQueue* commandQueue)
	{
		frameTools.transferCommandAllocator->Reset();
		frameTools.transferCommandList->Reset(frameTools.transferCommandAllocator.Get(), nullptr);

		// Transitions SSBOs to copy dest
		D3D12_RESOURCE_BARRIER copyDestBarriers[Ce_ConstDataSSBOCount]{};
		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_VertexStagingBufferIndex], buffers.vertexBuffer.buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_IndexStagingBufferIndex], buffers.indexBuffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_SurfaceStagingBufferIndex], buffers.surfaceBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_RenderStagingBufferIndex], buffers.renderBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_LodStagingIndex], buffers.lodBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_MaterialStagingIndex], buffers.materialBuffer.buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		frameTools.transferCommandList->ResourceBarrier(Ce_ConstDataSSBOCount, copyDestBarriers);

		// Transitions stagingBuffers to copy source
		D3D12_RESOURCE_BARRIER copySourceBarriers[Ce_ConstDataSSBOCount]{};
		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_VertexStagingBufferIndex], context.stagingBuffers[Ce_VertexStagingBufferIndex], 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_IndexStagingBufferIndex], context.stagingBuffers[Ce_IndexStagingBufferIndex],
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_SurfaceStagingBufferIndex], context.stagingBuffers[Ce_SurfaceStagingBufferIndex],
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_RenderStagingBufferIndex], context.stagingBuffers[Ce_RenderStagingBufferIndex],
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_LodStagingIndex], context.stagingBuffers[Ce_LodStagingIndex],
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_MaterialStagingIndex], context.stagingBuffers[Ce_MaterialStagingIndex],
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		frameTools.transferCommandList->ResourceBarrier(Ce_ConstDataSSBOCount, copySourceBarriers);

		frameTools.transferCommandList->CopyResource(buffers.vertexBuffer.buffer.Get(), context.stagingBuffers[Ce_VertexStagingBufferIndex]);
		frameTools.transferCommandList->CopyResource(buffers.indexBuffer.Get(), context.stagingBuffers[Ce_IndexStagingBufferIndex]);
		frameTools.transferCommandList->CopyResource(buffers.surfaceBuffer.buffer.Get(), context.stagingBuffers[Ce_SurfaceStagingBufferIndex]);
		frameTools.transferCommandList->CopyResource(buffers.renderBuffer.buffer.Get(), context.stagingBuffers[Ce_RenderStagingBufferIndex]);
		frameTools.transferCommandList->CopyResource(buffers.lodBuffer.buffer.Get(), context.stagingBuffers[Ce_LodStagingIndex]);
		frameTools.transferCommandList->CopyResource(buffers.materialBuffer.buffer.Get(), context.stagingBuffers[Ce_MaterialStagingIndex]);

		frameTools.transferCommandList->Close();
		ID3D12CommandList* commandLists[] = { frameTools.transferCommandList.Get() };
		commandQueue->ExecuteCommandLists(1, commandLists);

		const UINT64 fence = frameTools.copyFenceValue++;
		commandQueue->Signal(frameTools.copyFence.Get(), fence);
		if (frameTools.copyFence->GetCompletedValue() < fence)
		{
			frameTools.copyFence->SetEventOnCompletion(fence, frameTools.copyFenceEvent);
			WaitForSingleObject(frameTools.copyFenceEvent, INFINITE);
		}

		return 1;
	}

	static uint8_t CreateConstBuffers(ID3D12Device* device, Dx12Renderer::FrameTools& frameTools, ID3D12CommandQueue* commandQueue,
		BlitzenEngine::DrawContext& context, Dx12Renderer::ConstBuffers& buffers)
	{
		const auto& vertices{ context.m_meshes.m_hlslVtxs };
		const auto& indices{ context.m_meshes.m_indices };
		const auto& surfaces{ context.m_meshes.m_surfaces };
		auto renders{ context.m_renders.m_renders};
		auto renderObjectCount{ context.m_renders.m_renderCount };
		const auto& lods{ context.m_meshes.m_LODs};
		auto materials{ context.m_textures.m_materials };
		auto materialCount{ context.m_textures.m_materialCount };

		DX12WRAPPER<ID3D12Resource> vertexStagingBuffer{ nullptr };
		UINT64 vertexBufferSize{ CreateSSBO(device, buffers.vertexBuffer, vertexStagingBuffer, vertices.GetSize(), vertices.Data())};
		if (!vertexBufferSize)
		{
			BLIT_ERROR("Failed to create vertex buffer");
			return 0;
		}

		DX12WRAPPER<ID3D12Resource> indexStagingBuffer{ nullptr };
		UINT64 indexBufferSize{ CreateIndexBuffer(device, buffers.indexBuffer, indexStagingBuffer, indices.GetSize(), indices.Data(), buffers.indexBufferView)};
		if (!indexBufferSize)
		{
			BLIT_ERROR("Failed to create index buffer");
			return 0;
		}

		DX12WRAPPER<ID3D12Resource> surfaceStagingBuffer{ nullptr };
		UINT64 surfaceBufferSize{ CreateSSBO(device, buffers.surfaceBuffer, surfaceStagingBuffer, surfaces.GetSize(), surfaces.Data()) };
		if (!surfaceBufferSize)
		{
			BLIT_ERROR("Failed to create surface buffer");
			return 0;
		}

		DX12WRAPPER<ID3D12Resource> renderStagingBuffer{ nullptr };
		UINT64 renderBufferSize{ CreateSSBO(device, buffers.renderBuffer, renderStagingBuffer, renderObjectCount, renders) };
		if(!renderBufferSize)
		{
			BLIT_ERROR("Failed to create render buffer");
			return 0;
		}

		DX12WRAPPER<ID3D12Resource> lodStaging{ nullptr };
		UINT64 lodBufferSize{ CreateSSBO(device, buffers.lodBuffer, lodStaging, lods.GetSize(), lods.Data()) };
		if (!lodBufferSize)
		{
			BLIT_ERROR("Failed to create lod buffer");
			return 0;
		}

		// No data for the material buffer yet
		DX12WRAPPER<ID3D12Resource> materialStaging{ nullptr };
		UINT64 materialBufferSize{ CreateSSBO(device, buffers.materialBuffer, materialStaging, materialCount, const_cast<BlitzenEngine::Material*>(materials)) }; 
		if(!materialBufferSize)
		{
			BLIT_ERROR("Failed to create material buffer");
			return 0;
		}

		SSBOCopyContext copyContext{};
		copyContext.stagingBuffers[Ce_VertexStagingBufferIndex] = vertexStagingBuffer.Get();
		copyContext.stagingBuffers[Ce_IndexStagingBufferIndex] = indexStagingBuffer.Get();
		copyContext.stagingBuffers[Ce_SurfaceStagingBufferIndex] = surfaceStagingBuffer.Get();
		copyContext.stagingBuffers[Ce_RenderStagingBufferIndex] = renderStagingBuffer.Get();
		copyContext.stagingBuffers[Ce_LodStagingIndex] = lodStaging.Get();
		copyContext.stagingBuffers[Ce_MaterialStagingIndex] = materialStaging.Get();
		copyContext.stagingBufferSizes[Ce_VertexStagingBufferIndex] = vertexBufferSize;
		copyContext.stagingBufferSizes[Ce_IndexStagingBufferIndex] = indexBufferSize;
		copyContext.stagingBufferSizes[Ce_SurfaceStagingBufferIndex] = surfaceBufferSize;
		copyContext.stagingBufferSizes[Ce_RenderStagingBufferIndex] = renderBufferSize;
		copyContext.stagingBufferSizes[Ce_LodStagingIndex] = lodBufferSize;
		copyContext.stagingBufferSizes[Ce_MaterialStagingIndex] = materialBufferSize;
		if (!CopyDataToSSBOs(device, frameTools, buffers, copyContext, commandQueue))
		{
			BLIT_ERROR("Failed to copy data to GPU");
			return 0;
		}

		// Success
		return 1;
	}

	static uint8_t ModifyTextureIndices(ID3D12Device* device, Dx12Renderer::FrameTools& tools, ID3D12CommandQueue* queue, uint32_t textureOffset,
		ID3D12Resource* matBuffer, DX12WRAPPER<ID3D12Resource>& staging, BlitzenEngine::Material* pMaterials, uint32_t materialCount)
	{
		for (uint32_t i = 0; i < materialCount; ++i)
		{
			auto& mat{ pMaterials[i] };

			mat.albedoTag += textureOffset;
			mat.normalTag += textureOffset;
			mat.specularTag += textureOffset;
			mat.emissiveTag += textureOffset;
		}

		size_t dataSize{ materialCount * sizeof(BlitzenEngine::Material) };
		if (!CreateBuffer(device, staging.ReleaseAndGetAddressOf(), dataSize, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_UPLOAD))
		{
			BLIT_ERROR("Failed to create material staging buffer for texture indices update");
			return 0;
		}
		void* pData{ nullptr };
		auto mapRes{ staging->Map(0, nullptr, &pData) };
		if (FAILED(mapRes))
		{
			return LOG_ERROR_MESSAGE_AND_RETURN(mapRes);
		}
		BlitzenCore::BlitMemCopy(pData, pMaterials, dataSize);
		staging->Unmap(0, nullptr);

		tools.transferCommandAllocator->Reset();
		tools.transferCommandList->Reset(tools.transferCommandAllocator.Get(), nullptr);

		D3D12_RESOURCE_BARRIER copyBarrier{};
		CreateResourcesTransitionBarrier(copyBarrier, staging.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		tools.transferCommandList->ResourceBarrier(1, &copyBarrier);

		tools.transferCommandList->CopyResource(matBuffer, staging.Get());

		tools.transferCommandList->Close();
		ID3D12CommandList* lists[]{ tools.transferCommandList.Get() };
		queue->ExecuteCommandLists(1, lists);

		PlaceFence(tools.copyFenceValue, queue, tools.copyFence.Get(), tools.copyFenceEvent);

		return 1;
	}

	static void CreateResourceViews(ID3D12Device* device, ID3D12DescriptorHeap* srvHeap, Dx12Renderer::DescriptorContext& descriptorContext,
		Dx12Renderer::FrameTools& tools, ID3D12CommandQueue* queue, Dx12Renderer::ConstBuffers& staticBuffers, Dx12Renderer::VarBuffers* varBuffers, 
		BlitzenEngine::DrawContext& context, UINT textureCount, DX2DTEX* pTextures, DX12WRAPPER<ID3D12Resource>* pDepthTargets, UINT drawWidth, 
		UINT drawHeight, ID3D12DescriptorHeap* samplerHeap)
	{
		const auto& vertices{ context.m_meshes.m_hlslVtxs };
		const auto& transforms{ context.m_renders.m_transforms };
		const auto& surfaces{ context.m_meshes.m_surfaces };
		auto pRenders{ context.m_renders.m_renders };
		auto renderCount{ context.m_renders.m_renderCount };
		const auto& lods{ context.m_meshes.m_LODs };
		auto pMaterials{ context.m_textures.m_materials };
		auto materialCount{ context.m_textures.m_materialCount };

		for (size_t i = 0; i < ce_framesInFlight; ++i)
		{
			// Saves the offset of this group of descriptors and begins
			descriptorContext.opaqueSrvOffset[i] = descriptorContext.srvHeapOffset;
			descriptorContext.opaqueSrvHandle[i] = descriptorContext.srvHandle;
			descriptorContext.opaqueSrvHandle[i].ptr += descriptorContext.opaqueSrvOffset[i] * descriptorContext.srvIncrementSize;

			auto& vars = varBuffers[i];

			CreateBufferShaderResourceView(device, staticBuffers.vertexBuffer.buffer.Get(), srvHeap->GetCPUDescriptorHandleForHeapStart(),
				descriptorContext.srvHeapOffset, staticBuffers.vertexBuffer.heapOffset[i], (UINT)vertices.GetSize(), sizeof(BlitzenEngine::Vertex));
		}

		// Teams of descriptors used by both graphics and compute pipelines
		for (size_t i = 0; i < ce_framesInFlight; ++i)
		{
			// Saves the offset of this group of descriptors and begins
			descriptorContext.sharedSrvOffset[i] = descriptorContext.srvHeapOffset;
			descriptorContext.sharedSrvHandle[i] = descriptorContext.srvHandle;
			descriptorContext.sharedSrvHandle[i].ptr += descriptorContext.sharedSrvOffset[i] * descriptorContext.srvIncrementSize;

			auto& vars = varBuffers[i];

			CreateBufferShaderResourceView(device, staticBuffers.surfaceBuffer.buffer.Get(), srvHeap->GetCPUDescriptorHandleForHeapStart(),
				descriptorContext.srvHeapOffset, staticBuffers.surfaceBuffer.heapOffset[i], (UINT)surfaces.GetSize(), sizeof(BlitzenEngine::PrimitiveSurface));

			CreateBufferShaderResourceView(device, vars.transformBuffer.buffer.Get(), srvHeap->GetCPUDescriptorHandleForHeapStart(),
				descriptorContext.srvHeapOffset, vars.transformBuffer.heapOffset, context.m_renders.m_transformCount, sizeof(BlitzenEngine::MeshTransform));

			CreateBufferShaderResourceView(device, staticBuffers.renderBuffer.buffer.Get(), srvHeap->GetCPUDescriptorHandleForHeapStart(),
				descriptorContext.srvHeapOffset, staticBuffers.renderBuffer.heapOffset[i], renderCount, sizeof(BlitzenEngine::RenderObject));

			// This shit need to be cleaned up at some point
			vars.viewDataBuffer.cbvDesc = {};
			vars.viewDataBuffer.cbvDesc.BufferLocation = vars.viewDataBuffer.buffer->GetGPUVirtualAddress();
			vars.viewDataBuffer.cbvDesc.SizeInBytes = sizeof(BlitzenEngine::CameraViewData);
			auto descriptorHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
			descriptorHandle.ptr += descriptorContext.srvHeapOffset * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			device->CreateConstantBufferView(&vars.viewDataBuffer.cbvDesc, descriptorHandle);
			descriptorContext.srvHeapOffset++;

			if (BlitzenCore::Ce_InstanceCulling && !CE_DX12OCCLUSION)// Instancing not supported for draw occlusion mode
			{
				CreateUnorderedAccessView(device, vars.drawInstBuffer.buffer.Get(), nullptr, srvHeap->GetCPUDescriptorHandleForHeapStart(),
					descriptorContext.srvHeapOffset, UINT(lods.GetSize() * BlitzenCore::Ce_MaxInstanceCountPerLOD), sizeof(uint32_t), 0);
			}
		}

		// Teams of descriptors used for draw culling shaders
		for (size_t i = 0; i < ce_framesInFlight; ++i)
		{
			// Saves the offset of this group of descriptors and begins
			descriptorContext.cullSrvOffset[i] = descriptorContext.srvHeapOffset;
			descriptorContext.cullSrvHandle[i] = descriptorContext.srvHandle;
			descriptorContext.cullSrvHandle[i].ptr += descriptorContext.cullSrvOffset[i] * descriptorContext.srvIncrementSize;

			auto& vars = varBuffers[i];

			CreateUnorderedAccessView(device, vars.indirectDrawBuffer.buffer.Get(), vars.indirectDrawCount.buffer.Get(),
				srvHeap->GetCPUDescriptorHandleForHeapStart(), descriptorContext.srvHeapOffset, Ce_IndirectDrawCmdBufferSize, 
				sizeof(IndirectDrawCmd), 0);

			CreateUnorderedAccessView(device, vars.indirectDrawCount.buffer.Get(), nullptr, srvHeap->GetCPUDescriptorHandleForHeapStart(), 
				descriptorContext.srvHeapOffset, 1, sizeof(uint32_t), 0);

			CreateBufferShaderResourceView(device, staticBuffers.lodBuffer.buffer.Get(), srvHeap->GetCPUDescriptorHandleForHeapStart(),
				descriptorContext.srvHeapOffset, staticBuffers.lodBuffer.heapOffset[i], (UINT)lods.GetSize(), sizeof(BlitzenEngine::LodData));

			if constexpr (CE_DX12OCCLUSION)
			{
				CreateUnorderedAccessView(device, vars.drawVisibilityBuffer.buffer.Get(), nullptr, srvHeap->GetCPUDescriptorHandleForHeapStart(),
					descriptorContext.srvHeapOffset, renderCount, sizeof(uint32_t), 0);
			}
			else if constexpr (BlitzenCore::Ce_InstanceCulling)
			{
				CreateUnorderedAccessView(device, vars.lodInstBuffer.buffer.Get(), nullptr, srvHeap->GetCPUDescriptorHandleForHeapStart(),
					descriptorContext.srvHeapOffset, (UINT)lods.GetSize(), sizeof(BlitzenEngine::LodInstanceCounter), 0);
			}	
		}

		if constexpr (CE_DX12OCCLUSION)
		{
			CreateDepthPyramidDescriptors(device, varBuffers, descriptorContext, srvHeap, pDepthTargets, drawWidth, drawHeight, samplerHeap);
		}
		
		// material buffer, single srv bound to pixel shader
		descriptorContext.materialSrvOffset = descriptorContext.srvHeapOffset;
		descriptorContext.materialSrvHandle = descriptorContext.srvHandle;
		descriptorContext.materialSrvHandle.ptr += descriptorContext.materialSrvOffset * descriptorContext.srvIncrementSize;
		CreateBufferShaderResourceView(device, staticBuffers.materialBuffer.buffer.Get(), srvHeap->GetCPUDescriptorHandleForHeapStart(),
			descriptorContext.srvHeapOffset, staticBuffers.materialBuffer.heapOffset[0], (UINT)materialCount, sizeof(BlitzenEngine::Material));

		descriptorContext.texturesSrvOffset = descriptorContext.srvHeapOffset;
		descriptorContext.textureSrvHandle = descriptorContext.srvHandle;
		descriptorContext.textureSrvHandle.ptr += descriptorContext.texturesSrvOffset * descriptorContext.srvIncrementSize;
		// TEXTURES SHOULD BE THE LAST THING LOADED FOR THE SRV HEAP
		for (size_t i = 0; i < textureCount; ++i)
		{
			auto& tex2D{ pTextures[i] };

			CreateTextureShaderResourceView(device, tex2D.resource.Get(), srvHeap->GetCPUDescriptorHandleForHeapStart(),
				descriptorContext.srvHeapOffset, tex2D.format, tex2D.mipLevels);
		}
	}

	uint8_t Dx12Renderer::SetupForRendering(BlitzenEngine::DrawContext& context)
	{
		GenerateHlslVertices(context.m_meshes);

		if (!CreateRootSignatures(m_device.Get(), m_opaqueRootSignature.ReleaseAndGetAddressOf(), m_drawCullSignature.ReleaseAndGetAddressOf(), 
			m_drawCountResetRoot.ReleaseAndGetAddressOf(), m_drawOccLateSignature.ReleaseAndGetAddressOf(), m_depthPyramidSignature.ReleaseAndGetAddressOf()))
		{
			BLIT_ERROR("Failed to create root signatures");
			return 0;
		}

		if (!CreateCmdSignatures(m_device.Get(), m_opaqueRootSignature.Get(), m_opaqueCmdSingature.ReleaseAndGetAddressOf()))
		{
			BLIT_ERROR("Failed to create command signatures");
			return 0;
		}

		if (!CreateConstBuffers(m_device.Get(), m_frameTools[m_currentFrame], m_transferCommandQueue.Get(), context, m_constBuffers))
		{
			BLIT_ERROR("Failed to create constant buffers");
			return 0;
		}

		if (!CreateVarBuffers(m_device.Get(), m_transferCommandQueue.Get(), m_frameTools[m_currentFrame], m_varBuffers, context, 
			m_swapchainWidth, m_swapchainHeight))
		{
			BLIT_ERROR("Failed to create var buffers");
			return 0;
		}

		CreateResourceViews(m_device.Get(), m_srvHeap.Get(), m_descriptorContext, m_frameTools[m_currentFrame], m_transferCommandQueue.Get(), 
			m_constBuffers, m_varBuffers, context, m_textureCount, m_tex2DList, m_depthBuffers, m_swapchainWidth, m_swapchainHeight, 
			m_samplerHeap.Get());
		if (!CheckForDeviceRemoval(m_device.Get()))
		{
			BLIT_ERROR("Failed to create shader resource views");
			return 0;
		}

		PipelineCreationContext pipelineContext{ m_opaqueRootSignature.Get(), m_opaqueGraphicsPso.ReleaseAndGetAddressOf(), 
			m_drawCullSignature.Get(), m_drawCullPso.ReleaseAndGetAddressOf(), m_drawInstCmdPso.ReleaseAndGetAddressOf(), 
			m_drawInstCountResetPso.ReleaseAndGetAddressOf(), m_drawCountResetRoot.Get(), m_drawCountResetPso.ReleaseAndGetAddressOf(), 
			m_drawOccLateSignature.Get(), m_drawOccLatePso.ReleaseAndGetAddressOf(), m_depthPyramidSignature.Get(), m_depthPyramidPso.ReleaseAndGetAddressOf()};
		if (!CreatePipelines(m_device.Get(), pipelineContext))
		{
			BLIT_ERROR("Failed to create graphics pipelines");
			return 0;
		}

		// Gives the pyramid size to th camera. There is not much reason for the camera to have it, but it is what it is.
		context.m_camera.viewData.pyramidWidth = float(m_varBuffers[0].depthPyramid.width);
		context.m_camera.viewData.pyramidHeight = float(m_varBuffers[0].depthPyramid.height);

		return 1;
	}

	void Dx12Renderer::FinalSetup()
	{
		auto& frameTools{ m_frameTools[m_currentFrame] };

		frameTools.mainGraphicsCommandAllocator->Reset();
		frameTools.mainGraphicsCommandList->Reset(frameTools.mainGraphicsCommandAllocator.Get(), nullptr);

		D3D12_RESOURCE_BARRIER staticBufferBarriers[Ce_ConstDataSSBOCount]{};
		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_VertexStagingBufferIndex], m_constBuffers.vertexBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_IndexStagingBufferIndex], m_constBuffers.indexBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_SurfaceStagingBufferIndex], m_constBuffers.surfaceBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_RenderStagingBufferIndex], m_constBuffers.renderBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_LodStagingIndex], m_constBuffers.lodBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_MaterialStagingIndex], m_constBuffers.materialBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		frameTools.mainGraphicsCommandList->ResourceBarrier(Ce_ConstDataSSBOCount, staticBufferBarriers);

		/* Creates a barrier for each copy of a var buffer. Uses varBufferId to keep track of the array element */
		uint32_t varBufferId = 0;
		BlitCL::DynamicArray<D3D12_RESOURCE_BARRIER> varBuffersFinalState{ Ce_VarBuffersCount };
		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			CreateResourcesTransitionBarrier(varBuffersFinalState[varBufferId], m_varBuffers[i].viewDataBuffer.buffer.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ); // Might want to change this design later
			varBufferId++;
		}

		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			// Starts off as indirect argument, because the first transition barrier will be expecting that
			CreateResourcesTransitionBarrier(varBuffersFinalState[varBufferId], m_varBuffers[i].indirectDrawBuffer.buffer.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			varBufferId++;
		}

		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			// Starts off as indirect argument, because the first transition barrier will be expecting that
			CreateResourcesTransitionBarrier(varBuffersFinalState[varBufferId], m_varBuffers[i].indirectDrawCount.buffer.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			varBufferId++;
		}

		if constexpr (CE_DX12OCCLUSION)
		{
			for (uint32_t i = 0; i < ce_framesInFlight; ++i)
			{
				D3D12_RESOURCE_BARRIER drawVisibilityBarrier{};
				CreateResourcesTransitionBarrier(drawVisibilityBarrier, m_varBuffers[i].drawVisibilityBuffer.buffer.Get(),
					D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				varBuffersFinalState.PushBack(drawVisibilityBarrier);
			}

			for (uint32_t f = 0; f < ce_framesInFlight; ++f)
			{
				for (uint32_t p = 0; p < m_varBuffers[f].depthPyramid.mipCount; ++p)
				{
					D3D12_RESOURCE_BARRIER depthPyramidBarrier{};
					CreateResourcesTransitionBarrier(depthPyramidBarrier, m_varBuffers[f].depthPyramid.pyramid.Get(),
						D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, p);
					varBuffersFinalState.PushBack(depthPyramidBarrier);
				}
			}
		}
		else if constexpr (BlitzenCore::Ce_InstanceCulling)
		{
			for (uint32_t i = 0; i < ce_framesInFlight; ++i)
			{
				D3D12_RESOURCE_BARRIER drawInstBufferBarrier{};
				CreateResourcesTransitionBarrier(drawInstBufferBarrier, m_varBuffers[i].drawInstBuffer.buffer.Get(),
					D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				varBuffersFinalState.PushBack(drawInstBufferBarrier);
			}

			for (uint32_t i = 0; i < ce_framesInFlight; ++i)
			{
				D3D12_RESOURCE_BARRIER lodInstBufferBarrier{};
				CreateResourcesTransitionBarrier(lodInstBufferBarrier, m_varBuffers[i].lodInstBuffer.buffer.Get(),
					D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				varBuffersFinalState.PushBack(lodInstBufferBarrier);
			}
		}

		frameTools.mainGraphicsCommandList->ResourceBarrier((UINT)varBuffersFinalState.GetSize(), varBuffersFinalState.Data());

		for (uint32_t i = 0; i < m_textureCount; ++i)
		{
			D3D12_RESOURCE_BARRIER textureFinalBarrier{};
			CreateResourcesTransitionBarrier(textureFinalBarrier, m_tex2DList[i].resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			frameTools.mainGraphicsCommandList->ResourceBarrier(1, &textureFinalBarrier);
		}

		frameTools.mainGraphicsCommandList->Close();
		ID3D12CommandList* commandLists[] = { frameTools.mainGraphicsCommandList.Get() };
		m_commandQueue->ExecuteCommandLists(1, commandLists);

		const UINT64 fence = frameTools.inFlightFenceValue;
		m_commandQueue->Signal(frameTools.inFlightFence.Get(), fence);
		frameTools.inFlightFenceValue++;
		if (frameTools.inFlightFence->GetCompletedValue() < fence)
		{
			frameTools.inFlightFence->SetEventOnCompletion(fence, frameTools.inFlightFenceEvent);
			WaitForSingleObject(frameTools.inFlightFenceEvent, INFINITE);
		}
	}
}