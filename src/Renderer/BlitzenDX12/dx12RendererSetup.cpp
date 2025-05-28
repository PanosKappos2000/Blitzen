#if defined(_WIN32)

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

	static uint8_t LoadDDSImageData(BlitzenEngine::DDS_HEADER& header, BlitzenEngine::DDS_HEADER_DXT10& header10, BlitzenPlatform::C_FILE_SCOPE& scopedFILE, DXGI_FORMAT& format, void* pData, uint32_t& blockSize)
	{
		format = GetDDSFormat(header, header10);
		if (format == DXGI_FORMAT_UNKNOWN)
		{
			BLIT_ERROR("Unknow format retrieved from DDS image");
			return 0;
		}

		auto file = scopedFILE.m_pHandle;

		blockSize = BlitzenEngine::GetDDSBlockSize(header, header10);
		auto imageSize = BlitzenEngine::GetDDSImageSizeBC(header.dwWidth, header.dwHeight, header.dwMipMapCount, blockSize);

		size_t readSize = fread(pData, 1, imageSize, file);

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
		DXGI_FORMAT format, UINT blockSize, CmdContext& cmdContext, ID3D12CommandQueue* commandQueue, DX12WRAPPER<ID3D12Resource>& staging)
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

		cmdContext.m_copyCmdAlloc->Reset();
		cmdContext.m_copyCmdList->Reset(cmdContext.m_copyCmdAlloc.Get(), nullptr);

		// Transition texture to COPY_DEST state
		D3D12_RESOURCE_BARRIER preCopyBarriers[2]{};
		CreateResourcesTransitionBarrier(preCopyBarriers[0], resource.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		CreateResourcesTransitionBarrier(preCopyBarriers[1], staging.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		cmdContext.m_copyCmdList->ResourceBarrier(BLIT_ARRAY_SIZE(preCopyBarriers), preCopyBarriers);

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

			cmdContext.m_copyCmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, &box);

			bufferOffset += ((mipWidth + 3) / 4) * ((mipHeight + 3) / 4) * blockSize;
			mipWidth = mipWidth > 1 ? mipWidth / 2 : 1;
			mipHeight = mipHeight > 1 ? mipHeight / 2 : 1;
		}

		cmdContext.m_copyCmdList->Close();
		ID3D12CommandList* commandLists[] = { cmdContext.m_copyCmdList.Get() };
		commandQueue->ExecuteCommandLists(1, commandLists);

		PlaceFence(cmdContext.m_copyFence.m_value, commandQueue, cmdContext.m_copyFence.m_dx12Handle.Get(), cmdContext.m_copyFence.m_event);

		return 1;
	}

	uint8_t Dx12Renderer::UploadTexture(const char* filepath)
	{
		BlitzenEngine::DDS_HEADER header{};
		BlitzenEngine::DDS_HEADER_DXT10 header10{};
		BlitzenPlatform::C_FILE_SCOPE scopedFILE{};

		// Opens file
		if (!BlitzenEngine::OpenDDSImageFile(filepath, header, header10, scopedFILE))
		{
			BLIT_ERROR("Failed to open texture file");
			return 0;
		}

		// Copies texture data to staging resource
		DX12WRAPPER<ID3D12Resource> stagingBuffer;
		if (!CreateBuffer(m_device.Get(), stagingBuffer.ReleaseAndGetAddressOf(), Ce_TextureDataStagingSize, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_UPLOAD))
		{
			BLIT_ERROR("Failed to create staging buffer for texture data copy");
			return 0;
		}

		void* pData{ nullptr };
		HRESULT mappingRes = stagingBuffer->Map(0, nullptr, &pData);
		if (FAILED(mappingRes))
		{
			BLIT_ERROR("Failed to map pointer to texture staging buffer");
			return LOG_ERROR_MESSAGE_AND_RETURN(mappingRes);
		}

		auto& tex2D{ m_roResources.m_drawTextures[m_roResources.m_textureCount] };
		uint32_t blockSize{ 0 };

		// LOAD
		if (!LoadDDSImageData(header, header10, scopedFILE, tex2D.format, pData, blockSize))
		{
			BLIT_ERROR("Failed to load texture data");
			return 0;
		}

		tex2D.mipLevels = header.dwMipMapCount;
		if (!Create2DTexture(m_device.Get(), tex2D.resource, header.dwWidth, header.dwHeight, tex2D.mipLevels, tex2D.format, blockSize, m_cmdContext[m_currentFrame], 
			m_transferCommandQueue.Get(), stagingBuffer))
		{
			BLIT_ERROR("Failed to upload texture to GPU");
			return 0;
		}

		stagingBuffer->Unmap(0, nullptr);

		m_roResources.m_textureCount++;

		return 1;
	}

	static uint8_t CreateRootSignatures(ID3D12Device* device, PipelineContext& context)
	{
		// Range for descriptor table for SRVs that are allocated in the exclusive region of the heap for this root
		D3D12_DESCRIPTOR_RANGE opaqueSrvRanges[Ce_OpaqueDrawExclusiveSRVsRangeCount]{};
		CreateDescriptorRange(opaqueSrvRanges[Ce_OpaqueDrawVtxSRVRangeId], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_OpaqueDrawVtxxSRVRegister);

		// Ranges for descrtiptor table for SRVs that are allocated in the shared section of the heap
		D3D12_DESCRIPTOR_RANGE sharedSrvRanges[Ce_SharedSRVsRangeCount]{};
		CreateDescriptorRange(sharedSrvRanges[Ce_SurfaceSRVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_SurfaceSRVRegister);
		CreateDescriptorRange(sharedSrvRanges[Ce_TransformSRVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_TransformSRVRegister);
		CreateDescriptorRange(sharedSrvRanges[Ce_RenderSRVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_RenderSRVRegister);
		CreateDescriptorRange(sharedSrvRanges[Ce_ViewCBVRootID], D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, Ce_ViewCBVRegister);

		// Texture sampler
		D3D12_DESCRIPTOR_RANGE textureSamplerRange{};
		CreateDescriptorRange(textureSamplerRange, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, Ce_OpaqueDrawTexSMPRegister);

		// Range for material buffer (pixel shader only)
		D3D12_DESCRIPTOR_RANGE materialSrvRange{};
		CreateDescriptorRange(materialSrvRange, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_OpaqueDrawMatSRVRegister);

		// Range for textures
		D3D12_DESCRIPTOR_RANGE textureSrvsRange{};
		CreateDescriptorRange(textureSrvsRange, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Ce_OpaqueDrawTexDescriptorCount, Ce_OpaqueDrawTexRegister);

		// ROOT PARAMS
		D3D12_ROOT_PARAMETER opaqueDrawRootParams[Ce_OpaqueDrawRootParameterCount]{};
		CreateRootParameterDescriptorTable(opaqueDrawRootParams[Ce_OpaqueDrawExclusiveSRVsRootID], opaqueSrvRanges, Ce_OpaqueDrawExclusiveSRVsRangeCount, D3D12_SHADER_VISIBILITY_VERTEX);
		CreateRootParameterDescriptorTable(opaqueDrawRootParams[Ce_OpaqueDrawSharedSRVsRootID], sharedSrvRanges, Ce_SharedSRVsRangeCount, D3D12_SHADER_VISIBILITY_VERTEX);
		CreateRootParameterPushConstants(opaqueDrawRootParams[Ce_OpaqueDrawObjIDRootID], Ce_OpaqueDrawObjIDConstantRegister, 0, Ce_OpaqueDrawObjIDConstant32BitCount, D3D12_SHADER_VISIBILITY_VERTEX);
		CreateRootParameterDescriptorTable(opaqueDrawRootParams[Ce_OpaqueDrawTexSMPRootID], &textureSamplerRange, 1, D3D12_SHADER_VISIBILITY_PIXEL);
		CreateRootParameterDescriptorTable(opaqueDrawRootParams[Ce_OpaqueDrawMatSRVRootID], &materialSrvRange, 1, D3D12_SHADER_VISIBILITY_PIXEL);
		CreateRootParameterDescriptorTable(opaqueDrawRootParams[Ce_OpaqueDrawTexSRVRootID], &textureSrvsRange, 1, D3D12_SHADER_VISIBILITY_PIXEL);

		// OPAQUE DRAW ROOT
		if (!CreateRootSignature(device, context.m_opaqueDrawRoot.ReleaseAndGetAddressOf(), Ce_OpaqueDrawRootParameterCount, opaqueDrawRootParams, D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED))
		{
			BLIT_ERROR("Failed to create opaque root signature");
			return 0;
		}

		// Range for descriptor table for SRVs that are allocated in the exclusive region of the heap for this root
		D3D12_DESCRIPTOR_RANGE drawCullSrvRanges[Ce_DrawCullSRVsRangeCount]{};
		CreateDescriptorRange(drawCullSrvRanges[Ce_DrawCullDrawCmdUAVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, Ce_DrawCullDrawCmdUAVRegister);
		CreateDescriptorRange(drawCullSrvRanges[Ce_DrawCullDrawCmdCountUAVRegister], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, Ce_DrawCullDrawCmdCountUAVRegister);
		CreateDescriptorRange(drawCullSrvRanges[Ce_DrawCullLODSRVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_DrawCullLODSRVRegister);

		// ROOT PARAMS
		D3D12_ROOT_PARAMETER drawCullRootParameters[Ce_DrawCullRootParameterCount]{};
		CreateRootParameterDescriptorTable(drawCullRootParameters[Ce_DrawCullExclusiveSRVsRootID], drawCullSrvRanges, Ce_DrawCullSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
		CreateRootParameterDescriptorTable(drawCullRootParameters[Ce_DrawCullSharedSRVsRootID], sharedSrvRanges, Ce_SharedSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
		CreateRootParameterPushConstants(drawCullRootParameters[Ce_DrawCullDrawCountConstantRootID], Ce_DrawCullDrawCountContantRegister, 0, Ce_DrawCullDrawCountContant32BitCount, D3D12_SHADER_VISIBILITY_ALL);

		// DRAW CULL ROOT
		if (!CreateRootSignature(device, context.m_drawCullRoot.ReleaseAndGetAddressOf(), Ce_DrawCullRootParameterCount, drawCullRootParameters))
		{
			BLIT_ERROR("Failed to create draw cull root signature");
			return 0;
		}

		D3D12_ROOT_PARAMETER resetShaderRootParameter{};
		CreateRootParameterDescriptorTable(resetShaderRootParameter, drawCullSrvRanges, Ce_DrawCullSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);

		// DRAW COUNT RESET ROOT
		if (!CreateRootSignature(device, context.m_drawCountResetRoot.ReleaseAndGetAddressOf(), 1, &resetShaderRootParameter))
		{
			BLIT_ERROR("Failed to create draw count reset shader push constant");
			return 0;
		}

		// DRAW INST ADDITIONAL
		if constexpr (BlitzenCore::Ce_InstanceCulling)
		{
			// Range for inst buffer
			D3D12_DESCRIPTOR_RANGE instanceBufferRange{};
			CreateDescriptorRange(instanceBufferRange, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, Ce_OpaqueDrawInstInstUAVRegister);

			// ROOT PARAMS
			D3D12_ROOT_PARAMETER opaqueDrawInstRootParams[Ce_OpaqueDrawInstRootParameterCount]{};
			CreateRootParameterDescriptorTable(opaqueDrawInstRootParams[Ce_OpaqueDrawInstExclusiveSRVsRootID], opaqueSrvRanges, Ce_OpaqueDrawExclusiveSRVsRangeCount, D3D12_SHADER_VISIBILITY_VERTEX);
			CreateRootParameterDescriptorTable(opaqueDrawInstRootParams[Ce_OpaqueDrawInstSharedSRVsRootID], sharedSrvRanges, Ce_SharedSRVsRangeCount, D3D12_SHADER_VISIBILITY_VERTEX);
			CreateRootParameterPushConstants(opaqueDrawInstRootParams[Ce_OpaqueDrawObjIDRootID], Ce_OpaqueDrawObjIDConstantRegister, 0, Ce_OpaqueDrawObjIDConstant32BitCount, D3D12_SHADER_VISIBILITY_VERTEX);
			CreateRootParameterDescriptorTable(opaqueDrawInstRootParams[Ce_OpaqueDrawInstTexSMPRootID], &textureSamplerRange, 1, D3D12_SHADER_VISIBILITY_PIXEL);
			CreateRootParameterDescriptorTable(opaqueDrawInstRootParams[Ce_OpaqueDrawInstMatSRVRootID], &materialSrvRange, 1, D3D12_SHADER_VISIBILITY_PIXEL);
			CreateRootParameterDescriptorTable(opaqueDrawInstRootParams[Ce_OpaqueDrawInstTexSRVRootID], &textureSrvsRange, 1, D3D12_SHADER_VISIBILITY_PIXEL);
			CreateRootParameterDescriptorTable(opaqueDrawInstRootParams[Ce_OpaqueDrawInstInstSRVRootID], &instanceBufferRange, 1, D3D12_SHADER_VISIBILITY_VERTEX);

			// OPAQUE DRAW INST ROOT
			if (!CreateRootSignature(device, context.m_opaqueDrawInstRoot.ReleaseAndGetAddressOf(), Ce_OpaqueDrawInstRootParameterCount, opaqueDrawInstRootParams, D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED))
			{
				BLIT_ERROR("Failed to create opaque root signature");
				return 0;
			}

			// Draw cull inst additional ranges
			D3D12_DESCRIPTOR_RANGE drawCullInstRanges[Ce_DrawCullInstSRVsRangeCount]{};
			CreateDescriptorRange(drawCullInstRanges[Ce_DrawCullInstInstIdsxUAVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, Ce_DrawCullInstInstIdsxUAVRegister);
			CreateDescriptorRange(drawCullInstRanges[Ce_DrawCullInstInstCounterUAVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, Ce_DrawCullInstInstCounterUAVRegister);

			D3D12_ROOT_PARAMETER drawInstCullRootParameters[Ce_DrawCullInstRootParameterCount]{};
			CreateRootParameterDescriptorTable(drawInstCullRootParameters[Ce_DrawCullInstExclusiveSRVsRootID], drawCullSrvRanges, Ce_DrawCullSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(drawInstCullRootParameters[Ce_DrawCullInstSharedSRVsRootID], sharedSrvRanges, Ce_SharedSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterPushConstants(drawInstCullRootParameters[Ce_DrawCullInstDrawCountConstantRootID], Ce_DrawCullDrawCountContantRegister, 0, Ce_DrawCullDrawCountContant32BitCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(drawInstCullRootParameters[Ce_DrawCullInstAdditionalSRVsRootID], drawCullInstRanges, Ce_DrawCullInstSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);

			// DRAW CULL INST ROOT
			if (!CreateRootSignature(device, context.m_drawCullInstRoot.ReleaseAndGetAddressOf(), Ce_DrawCullInstRootParameterCount, drawInstCullRootParameters))
			{
				BLIT_ERROR("Failed to create draw cull inst root signature");
				return 0;
			}
		}

		// DRAW OCC ADDITIONAL
		if constexpr (CE_DX12OCCLUSION)
		{
			// Additional Draw visibility srv 
			D3D12_DESCRIPTOR_RANGE drawVisibilityBufferRange{};
			CreateDescriptorRange(drawVisibilityBufferRange, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, Ce_DrawOccFirstDrawVisUAVRegister);

			D3D12_ROOT_PARAMETER drawOccFirstRootParameters[Ce_DrawOccFirstRootParameterCount]{};
			CreateRootParameterDescriptorTable(drawOccFirstRootParameters[Ce_DrawOccFirstExclusiveSRVsRootId], drawCullSrvRanges, Ce_DrawCullSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(drawOccFirstRootParameters[Ce_DrawOccFirstSharedSRVsRootId], sharedSrvRanges, Ce_SharedSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterPushConstants(drawOccFirstRootParameters[Ce_DrawOccFirstDrawCountRootId], Ce_DrawCullDrawCountContantRegister, 0, Ce_DrawCullDrawCountContant32BitCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(drawOccFirstRootParameters[Ce_DrawOccFirstDrawVisUAVRootId], &drawVisibilityBufferRange, 1, D3D12_SHADER_VISIBILITY_ALL);

			// DRAW OCC FIRST PASS ROOT
			if (!CreateRootSignature(device, context.m_drawOccFirstRoot.ReleaseAndGetAddressOf(), Ce_DrawOccFirstRootParameterCount, drawOccFirstRootParameters))
			{
				BLIT_ERROR("Failed to create draw occ first root signature");
				return 0;
			}

			// Additional Depth pyramid srv
			D3D12_DESCRIPTOR_RANGE depthPyramidCullRange{};
			CreateDescriptorRange(depthPyramidCullRange, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_DrawOccLateHI_Z_MapSRVRegister);

			D3D12_ROOT_PARAMETER drawOccLateRootParameters[Ce_DrawOccLateRootParameterCount]{};
			CreateRootParameterDescriptorTable(drawOccLateRootParameters[Ce_DrawOccLateExclusiveSRVsRootId], drawCullSrvRanges, Ce_DrawCullSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(drawOccLateRootParameters[Ce_DrawOccLateSharedSRVsRootId], sharedSrvRanges, Ce_SharedSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterPushConstants(drawOccLateRootParameters[Ce_DrawOccLateDrawCountRootId], Ce_DrawCullDrawCountContantRegister, 0, Ce_DrawCullDrawCountContant32BitCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(drawOccLateRootParameters[Ce_DrawOccLateDrawVisUAVRootId], &drawVisibilityBufferRange, 1, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(drawOccLateRootParameters[Ce_DrawOccLateHI_Z_MapRootId], &depthPyramidCullRange, 1, D3D12_SHADER_VISIBILITY_ALL);

			// DRAW OCC LATE PASS ROOT
			if (!CreateRootSignature(device, context.m_drawOccLateRoot.ReleaseAndGetAddressOf(), Ce_DrawOccLateRootParameterCount, drawOccLateRootParameters))
			{
				BLIT_ERROR("Failed to create late cull (occlusion culling) root parameter");
				return 0;
			}
		}

		if constexpr (CE_DX12_BUILD_HI_Z_MAP)
		{
			// Additional for hi_z_map generation
			D3D12_DESCRIPTOR_RANGE depthPyramidUAVRange{};
			CreateDescriptorRange(depthPyramidUAVRange, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, Ce_HI_Z_MapUAVRegister);

			D3D12_DESCRIPTOR_RANGE depthPyramidSRVRange{};
			CreateDescriptorRange(depthPyramidSRVRange, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_HI_Z_MapSRVRegister);

			D3D12_ROOT_PARAMETER depthPyramidGenParameters[Ce_HI_Z_MapRootParameterCount]{};
			CreateRootParameterDescriptorTable(depthPyramidGenParameters[Ce_HI_Z_MapUAVRootID], &depthPyramidUAVRange, 1, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(depthPyramidGenParameters[Ce_HI_Z_MapSRVRootID], &depthPyramidSRVRange, 1, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterPushConstants(depthPyramidGenParameters[Ce_HI_Z_MapMipLvlConstantRootID], Ce_HI_Z_MapMipLvlConstantRegister, 0, Ce_HI_Z_MapMipLvlContant32BitCount, D3D12_SHADER_VISIBILITY_ALL);

			// HI_Z MAP ROOT
			if (!CreateRootSignature(device, context.m_HI_Z_MapRoot.ReleaseAndGetAddressOf(), Ce_HI_Z_MapRootParameterCount, depthPyramidGenParameters))
			{
				BLIT_ERROR("Failed to create depth pyramid root parameter");
				return 0;
			}
		}

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			D3D12_DESCRIPTOR_RANGE clusterDispatchAdditionalViewRanges[Ce_ClusterDispatchAdditionalViewsRangeCount]{};
			CreateDescriptorRange(clusterDispatchAdditionalViewRanges[Ce_ClusterDispatchCmdUAVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, Ce_ClusterDispatchCmdUAVRegister);
			CreateDescriptorRange(clusterDispatchAdditionalViewRanges[Ce_ClusterDispatchCmdCounterUAVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, Ce_ClusterDispatchCmdCounterUAVRegister);
			
			D3D12_DESCRIPTOR_RANGE depthPyramidCullRange{};
			CreateDescriptorRange(depthPyramidCullRange, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_DrawOccLateHI_Z_MapSRVRegister);

			D3D12_DESCRIPTOR_RANGE clusterSrvRange{};
			CreateDescriptorRange(clusterSrvRange, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_ClusterCullClusterSRVRegister);

			D3D12_ROOT_PARAMETER clusterCullRootParameters[Ce_ClusterCullRootParameterCount]{};
			CreateRootParameterDescriptorTable(clusterCullRootParameters[Ce_ClusterCullExclusiveSRVsRootID], drawCullSrvRanges, Ce_DrawCullSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(clusterCullRootParameters[Ce_ClusterCullSharedSRVsRootID], sharedSrvRanges, Ce_SharedSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterPushConstants(clusterCullRootParameters[Ce_ClusterCullDrawCountRootID], Ce_DrawCullDrawCountContantRegister, 0, Ce_DrawCullDrawCountContant32BitCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterPushConstants(clusterCullRootParameters[Ce_ClusterCullIdxDataConstantRootID], CE_ClusterCullIdsDataConstantRegister, 0, Ce_ClusterCullIdxDataConstant32BitCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(clusterCullRootParameters[Ce_ClusterCullAdditionalViewsRootID], clusterDispatchAdditionalViewRanges, Ce_ClusterDispatchAdditionalViewsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(clusterCullRootParameters[Ce_ClusterCullClusterSRVRootID], &clusterSrvRange, 1, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(clusterCullRootParameters[Ce_ClusterCullHI_Z_MapSrvRootID], &depthPyramidCullRange, 1, D3D12_SHADER_VISIBILITY_ALL);
		}

		// success
		return 1;
	}

	static uint8_t CreateCmdSignatures(ID3D12Device* device, PipelineContext& ctx)
	{
		// Draw command
		D3D12_INDIRECT_ARGUMENT_DESC indirectDescs[2]{};
		indirectDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

		// Object id root constant
		indirectDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
		indirectDescs[0].Constant.DestOffsetIn32BitValues = 0;
		indirectDescs[0].Constant.Num32BitValuesToSet = Ce_OpaqueDrawObjIDConstant32BitCount;
		indirectDescs[0].Constant.RootParameterIndex = Ce_OpaqueDrawObjIDRootID;

		D3D12_COMMAND_SIGNATURE_DESC sigDesc{};
		sigDesc.NodeMask = 0;
		sigDesc.NumArgumentDescs = 2;
		sigDesc.pArgumentDescs = indirectDescs;
		sigDesc.ByteStride = sizeof(IndirectDrawCmd);

		// regular opaque draw cmd signature
		HRESULT opaqueCmdRes{ device->CreateCommandSignature(&sigDesc, ctx.m_opaqueDrawRoot.Get(), IID_PPV_ARGS(ctx.m_opaqueDrawCmdSign.ReleaseAndGetAddressOf()))};
		if (FAILED(opaqueCmdRes))
		{
			BLIT_ERROR("Failed to create opaque draw command signature");
			return LOG_ERROR_MESSAGE_AND_RETURN(opaqueCmdRes);
		}

		// opaque draw inst cmd signature
		if constexpr (BlitzenCore::Ce_InstanceCulling)
		{
			HRESULT opaqueInstCmdRes{ device->CreateCommandSignature(&sigDesc, ctx.m_opaqueDrawInstRoot.Get(), IID_PPV_ARGS(ctx.m_opaqueDrawInstCmdSign.ReleaseAndGetAddressOf())) };
			if (FAILED(opaqueInstCmdRes))
			{
				BLIT_ERROR("Failed to create opaque draw instanced command signature");
				return LOG_ERROR_MESSAGE_AND_RETURN(opaqueInstCmdRes);
			}
		}

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			D3D12_INDIRECT_ARGUMENT_DESC clusterDispatchIndirectDescs[2]{};
			
			clusterDispatchIndirectDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
			clusterDispatchIndirectDescs[0].Constant.DestOffsetIn32BitValues = 0;
			clusterDispatchIndirectDescs[0].Constant.Num32BitValuesToSet = Ce_ClusterCullIdxDataConstant32BitCount;
			clusterDispatchIndirectDescs[0].Constant.RootParameterIndex = Ce_ClusterCullIdxDataConstantRootID;

			clusterDispatchIndirectDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

			D3D12_COMMAND_SIGNATURE_DESC clusterSigDesc{};
			clusterSigDesc.NodeMask = 0;
			clusterSigDesc.NumArgumentDescs = BLIT_ARRAY_SIZE(clusterDispatchIndirectDescs);
			clusterSigDesc.pArgumentDescs = clusterDispatchIndirectDescs;
			clusterSigDesc.ByteStride = sizeof(ClusterDispatchCmd);

			HRESULT clusterDispatchCmdRes{ device->CreateCommandSignature(&clusterSigDesc, ctx.m_clusterCullRoot.Get(), IID_PPV_ARGS(ctx.m_clusterCullCmdSign.ReleaseAndGetAddressOf())) };
			if (FAILED(clusterDispatchCmdRes))
			{
				BLIT_ERROR("Failed to create cluster dispatch command signature");
				return LOG_ERROR_MESSAGE_AND_RETURN(clusterDispatchCmdRes);
			}
		}

		// success
		return 1;
	}

	static uint8_t CreatePipelines(ID3D12Device* device, PipelineContext& context)
	{
		if (!CreateOpaqueGraphicsPipeline(device, context))
		{
			BLIT_ERROR("Failed to create opaque grahics pipeline");
			return 0;
		}

		if (!CreateComputeShaderProgram(device, context.m_drawCountResetRoot.Get(), context.m_drawCountResetPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawCountReset.cs.hlsl.bin"))
		{
			BLIT_ERROR("Failed to create drawCountReset.cs shader program");
			return 0;
		}

		if (!CreateComputeShaderProgram(device, context.m_drawCullRoot.Get(), context.m_drawCullPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawCull.cs.hlsl.bin"))
		{
			BLIT_ERROR("Failed to create drawCull.cs shader program");
			return 0;
		}

		if constexpr (CE_DX12_BUILD_HI_Z_MAP)
		{
			if (!CreateComputeShaderProgram(device, context.m_HI_Z_MapRoot.Get(), context.m_HI_Z_MapPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/hi_z_map.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create depthPyramid.cs shader program");
				return 0;
			}
		}

		if constexpr (CE_DX12OCCLUSION)
		{
			if (!CreateComputeShaderProgram(device, context.m_drawOccFirstRoot.Get(), context.m_drawOccFirstPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawOccFirst.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create drawCull.cs shader program");
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.m_drawOccLateRoot.Get(), context.m_drawOccLatePso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawOccLate.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create drawOccLate.cs shader program");
				return 0;
			}
		}

		if constexpr (CE_DX12TEMPORAL_OCCLUSION)
		{
			if (!CreateComputeShaderProgram(device, context.m_drawOccLateRoot.Get(), context.m_drawOccTemporalPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawOccTemporal.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create drawOccTemporal.cs shader program");
				return 0;
			}
		}

		if constexpr (BlitzenCore::Ce_InstanceCulling)
		{
			if (!CreateComputeShaderProgram(device, context.m_drawCullInstRoot.Get(), context.m_drawCullInstPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawInstCull.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create drawInstCull.cs shader program");
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.m_drawCullInstRoot.Get(), context.m_drawInstCmdPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawInstCmd.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create drawInstCull.cs shader program");
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.m_drawCullInstRoot.Get(), context.m_drawInstCountResetPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawInstCountReset.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create drawInstCountReset.cs shader program");
				return 0;
			}
		}

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			if (!CreateComputeShaderProgram(device, context.m_clusterCullRoot.Get(), context.m_clusterCullDispatchPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/clusterCullDispatch.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create clusterCullDispatch.cs shader program");
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.m_clusterCullRoot.Get(), context.m_clusterCullPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/clusterCull.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create clusterCull.cs shader program");
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.m_clusterCullRoot.Get(), context.m_clusterDispatchCountResetPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/clusterDispatchCountReset.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create clusterDispatchCountReset.cs shader program");
				return 0;
			}
		}

		return 1;
	}

	static uint8_t CreateVarBuffers(ID3D12Device* device, ID3D12CommandQueue* commandQueue, CmdContext& cmdContext, 
		ReadWriteResources* rwResourcesArray, BlitzenEngine::DrawContext& context, uint32_t swapchainWidth, uint32_t swapchainHeight)
	{
		const auto& transforms{ context.m_renders.m_transforms };
		const auto& lodData{ context.m_meshes.m_LODs };
		const auto& lodInstanceList{ context.m_meshes.m_lodInstanceList };

		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			auto& rwResources = rwResourcesArray[i];

			if (!CreateCBuffer(device, rwResources.m_viewBuffer))
			{
				BLIT_ERROR("Failed to create view data buffer");
				return 0;
			}

			DX12WRAPPER<ID3D12Resource> transformStaging;
			if (!CreateCPUDataSSBO(device, rwResources.m_transformBuffer, transformStaging, context.m_renders.m_transformCount, context.m_renders.m_transforms, 
				context.m_renders.m_dynamicTransformCount))
			{
				BLIT_ERROR("Failed to create transform buffer");
				return 0;
			}

			UINT64 indirectBufferSize{ Ce_IndirectDrawCmdBufferSize * sizeof(IndirectDrawCmd) };
			if (!CreateBuffer(device, rwResources.m_drawCmdBuffer.buffer.ReleaseAndGetAddressOf(), indirectBufferSize, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_DEFAULT,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
			{
				BLIT_ERROR("Failed to create indirect draw buffer");
				return 0;
			}

			if (!CreateBuffer(device, rwResources.m_drawCmdCounterBuffer.buffer.ReleaseAndGetAddressOf(), sizeof(uint32_t), D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_DEFAULT, 
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
			{
				BLIT_ERROR("Failed to create indirect count buffer");
				return 0;
			}

			DX12WRAPPER<ID3D12Resource> drawVisibilityStaging{ nullptr };
			UINT64 visibilityBufferSize{ 0 };

			// DRAW OCC MODE
			if constexpr (CE_DX12OCCLUSION)
			{
				BlitCL::DynamicArray<uint32_t> zeroData{ context.m_renders.m_renderCount, 0 };
				
				// Normally only needed for non-temporal occlusion, but right now it gets created anyway, which is a bit of a waste
				visibilityBufferSize = CreateSSBO(device, rwResources.m_drawVisBuffer, drawVisibilityStaging, context.m_renders.m_renderCount, zeroData.Data(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
				if (!visibilityBufferSize)
				{
					BLIT_ERROR("Failed to create draw visibility buffer for draw occlusion");
					return 0;
				}
			}

			if constexpr (CE_DX12_BUILD_HI_Z_MAP)
			{
				if (!CreateDepthPyramidResource(device, rwResources.m_HI_Z, swapchainWidth, swapchainHeight))
				{
					BLIT_ERROR("Failed to create depth pyramid for occlusion culling");
					return 0;
				}
			}

			DX12WRAPPER<ID3D12Resource> lodInstStaging{ nullptr };
			UINT64 lodInstanceBufferSize{ 0 };

			// DRAW CULL INST MODE
			if constexpr (BlitzenCore::Ce_InstanceCulling)
			{
				UINT64 instanceBufferSize{ lodData.GetSize() * BlitzenCore::Ce_MaxInstanceCountPerLOD * sizeof(uint32_t) };
				if (!CreateBuffer(device, rwResources.m_drawInstBuffer.buffer.ReleaseAndGetAddressOf(), instanceBufferSize, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_DEFAULT, 
					D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
				{
					BLIT_ERROR("Failed to create instance buffer");
					return 0;
				}

				lodInstanceBufferSize = CreateSSBO(device, rwResources.m_instCounterBuffer, lodInstStaging, lodInstanceList.GetSize(), lodInstanceList.Data(),
					D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
				if (!lodInstanceBufferSize)
				{
					BLIT_ERROR("Failed to create lod instance counter buffer");
					return 0;
				}
			}

			// CLUSTER CULL MODE
			if constexpr (BlitzenCore::Ce_BuildClusters)
			{

			}

			// DATA COPY
			cmdContext.m_copyCmdAlloc->Reset();
			cmdContext.m_copyCmdList->Reset(cmdContext.m_copyCmdAlloc.Get(), nullptr);

			// DEST BARRIERS
			BlitCL::DynamicArray<D3D12_RESOURCE_BARRIER> copyDestBarriers{ Ce_VarSSBODataCount };

			CreateResourcesTransitionBarrier(copyDestBarriers[0], rwResources.m_transformBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

			// DRAW OCC MODE VIS BUFFER (normally not needed for temporal occlusion)
			if constexpr (CE_DX12OCCLUSION)
			{
				D3D12_RESOURCE_BARRIER visibilityBufferDestBarrier{};
				CreateResourcesTransitionBarrier(visibilityBufferDestBarrier, rwResources.m_drawVisBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

				copyDestBarriers.PushBack(visibilityBufferDestBarrier);
			}

			// DRAW CULL INST MODE (instance counter buffer)
			if constexpr (BlitzenCore::Ce_InstanceCulling)
			{
				D3D12_RESOURCE_BARRIER lodInstBufferDestBarrier{};
				CreateResourcesTransitionBarrier(lodInstBufferDestBarrier, rwResources.m_instCounterBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

				copyDestBarriers.PushBack(lodInstBufferDestBarrier);
			}

			// Execute
			cmdContext.m_copyCmdList->ResourceBarrier((UINT)copyDestBarriers.GetSize(), copyDestBarriers.Data());

			// SRC BARRIERS
			BlitCL::DynamicArray<D3D12_RESOURCE_BARRIER> copySourceBarriers{ Ce_VarSSBODataCount };

			CreateResourcesTransitionBarrier(copySourceBarriers[0], transformStaging.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

			if constexpr (CE_DX12OCCLUSION)
			{
				D3D12_RESOURCE_BARRIER visibilityBufferSourceBarrier{};
				CreateResourcesTransitionBarrier(visibilityBufferSourceBarrier, drawVisibilityStaging.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
				copySourceBarriers.PushBack(visibilityBufferSourceBarrier);
			}

			if constexpr (BlitzenCore::Ce_InstanceCulling)
			{
				D3D12_RESOURCE_BARRIER lodInstBufferSourceBarrier{};
				CreateResourcesTransitionBarrier(lodInstBufferSourceBarrier, lodInstStaging.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
				copySourceBarriers.PushBack(lodInstBufferSourceBarrier);
			}

			// Execute
			cmdContext.m_copyCmdList->ResourceBarrier(UINT(copySourceBarriers.GetSize()), copySourceBarriers.Data());

			// transforms
			cmdContext.m_copyCmdList->CopyResource(rwResources.m_transformBuffer.buffer.Get(), transformStaging.Get());

			// visibilities zeroed
			if constexpr (CE_DX12OCCLUSION)
			{
				cmdContext.m_copyCmdList->CopyResource(rwResources.m_drawVisBuffer.buffer.Get(), drawVisibilityStaging.Get());
			}

			// instance counter
			if constexpr (BlitzenCore::Ce_InstanceCulling)
			{
				cmdContext.m_copyCmdList->CopyResource(rwResources.m_instCounterBuffer.buffer.Get(), lodInstStaging.Get());
			}

			// Puts persistent transform staging in copy source state forever
			D3D12_RESOURCE_BARRIER dynamicTransformBarrier{};
			CreateResourcesTransitionBarrier(dynamicTransformBarrier, rwResources.m_transformBuffer.staging.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
			// execute
			cmdContext.m_copyCmdList->ResourceBarrier(1, &dynamicTransformBarrier);

			cmdContext.m_copyCmdList->Close();
			ID3D12CommandList* commandLists[] = { cmdContext.m_copyCmdList.Get() };
			commandQueue->ExecuteCommandLists(1, commandLists);

			PlaceFence(cmdContext.m_copyFence.m_value, commandQueue, cmdContext.m_copyFence.m_dx12Handle.Get(), cmdContext.m_copyFence.m_event);
		}
		// Success
		return 1;
	}

	static uint8_t CreateConstBuffers(ID3D12Device* device, CmdContext& cmdContext, ID3D12CommandQueue* commandQueue, BlitzenEngine::DrawContext& context, ReadOnlyResources& roResources)
	{
		const auto& vertices{ context.m_meshes.m_hlslVtxs };
		const auto& indices{ context.m_meshes.m_indices };
		const auto& surfaces{ context.m_meshes.m_surfaces };
		const auto& lods{ context.m_meshes.m_LODs};

		DX12WRAPPER<ID3D12Resource> vertexStagingBuffer{ nullptr };
		UINT64 vertexBufferSize{ CreateSSBO(device, roResources.m_vtxBuffer, vertexStagingBuffer, vertices.GetSize(), vertices.Data())};
		if (!vertexBufferSize)
		{
			BLIT_ERROR("Failed to create vertex buffer");
			return 0;
		}

		DX12WRAPPER<ID3D12Resource> indexStagingBuffer{ nullptr };
		UINT64 indexBufferSize{ CreateIndexBuffer(device, roResources.m_idxBuffer, indexStagingBuffer, indices.GetSize(), indices.Data())};
		if (!indexBufferSize)
		{
			BLIT_ERROR("Failed to create index buffer");
			return 0;
		}

		DX12WRAPPER<ID3D12Resource> surfaceStagingBuffer{ nullptr };
		UINT64 surfaceBufferSize{ CreateSSBO(device, roResources.m_surfaceBuffer, surfaceStagingBuffer, surfaces.GetSize(), surfaces.Data()) };
		if (!surfaceBufferSize)
		{
			BLIT_ERROR("Failed to create surface buffer");
			return 0;
		}

		DX12WRAPPER<ID3D12Resource> renderStagingBuffer{ nullptr };
		UINT64 renderBufferSize{ CreateSSBO(device, roResources.m_renderBuffer, renderStagingBuffer, context.m_renders.m_renderCount, context.m_renders.m_renders) };
		if(!renderBufferSize)
		{
			BLIT_ERROR("Failed to create render buffer");
			return 0;
		}

		DX12WRAPPER<ID3D12Resource> lodStaging{ nullptr };
		UINT64 lodBufferSize{ CreateSSBO(device, roResources.m_LODBuffer, lodStaging, lods.GetSize(), lods.Data()) };
		if (!lodBufferSize)
		{
			BLIT_ERROR("Failed to create lod buffer");
			return 0;
		}

		// No data for the material buffer yet
		DX12WRAPPER<ID3D12Resource> materialStaging{ nullptr };
		UINT64 materialBufferSize{ CreateSSBO(device, roResources.m_matBuffer, materialStaging, context.m_textures.m_materialCount, context.m_textures.m_materials) }; 
		if(!materialBufferSize)
		{
			BLIT_ERROR("Failed to create material buffer");
			return 0;
		}

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{

		}

		// DATA COPIES
		cmdContext.m_copyCmdAlloc->Reset();
		cmdContext.m_copyCmdList->Reset(cmdContext.m_copyCmdAlloc.Get(), nullptr);

		// DEST BARRIERS
		D3D12_RESOURCE_BARRIER copyDestBarriers[Ce_ConstDataSSBOCount]{};

		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_VertexStagingBufferIndex], roResources.m_vtxBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_IndexStagingBufferIndex], roResources.m_idxBuffer.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		
		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_SurfaceStagingBufferIndex], roResources.m_surfaceBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		
		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_RenderStagingBufferIndex], roResources.m_renderBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		
		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_LodStagingIndex], roResources.m_LODBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		
		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_MaterialStagingIndex], roResources.m_matBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		
		// execute
		cmdContext.m_copyCmdList->ResourceBarrier(Ce_ConstDataSSBOCount, copyDestBarriers);

		// SRC BARRIERS
		D3D12_RESOURCE_BARRIER copySourceBarriers[Ce_ConstDataSSBOCount]{};

		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_VertexStagingBufferIndex], vertexStagingBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		
		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_IndexStagingBufferIndex], indexStagingBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		
		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_SurfaceStagingBufferIndex], surfaceStagingBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		
		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_RenderStagingBufferIndex], renderStagingBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		
		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_LodStagingIndex], lodStaging.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		
		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_MaterialStagingIndex], materialStaging.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		// execute
		cmdContext.m_copyCmdList->ResourceBarrier(Ce_ConstDataSSBOCount, copySourceBarriers);

		cmdContext.m_copyCmdList->CopyResource(roResources.m_vtxBuffer.buffer.Get(), vertexStagingBuffer.Get());
		cmdContext.m_copyCmdList->CopyResource(roResources.m_idxBuffer.m_buffer.Get(), indexStagingBuffer.Get());
		cmdContext.m_copyCmdList->CopyResource(roResources.m_surfaceBuffer.buffer.Get(), surfaceStagingBuffer.Get());
		cmdContext.m_copyCmdList->CopyResource(roResources.m_renderBuffer.buffer.Get(), renderStagingBuffer.Get());
		cmdContext.m_copyCmdList->CopyResource(roResources.m_LODBuffer.buffer.Get(), lodStaging.Get());
		cmdContext.m_copyCmdList->CopyResource(roResources.m_matBuffer.buffer.Get(), materialStaging.Get());

		cmdContext.m_copyCmdList->Close();
		ID3D12CommandList* commandLists[] = { cmdContext.m_copyCmdList.Get() };
		commandQueue->ExecuteCommandLists(1, commandLists);

		PlaceFence(cmdContext.m_copyFence.m_value, commandQueue, cmdContext.m_copyFence.m_dx12Handle.Get(), cmdContext.m_copyFence.m_event);

		// Success
		return 1;
	}

	static uint8_t ModifyTextureIndices(ID3D12Device* device, CmdContext& cmdContext, ID3D12CommandQueue* queue, uint32_t textureOffset,
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
		HRESULT mapRes{ staging->Map(0, nullptr, &pData) };
		if (FAILED(mapRes))
		{
			BLIT_ERROR("Failed to map pointer to mat staging buffer");
			return LOG_ERROR_MESSAGE_AND_RETURN(mapRes);
		}
		BlitzenCore::BlitMemCopy(pData, pMaterials, dataSize);
		staging->Unmap(0, nullptr);

		cmdContext.m_copyCmdAlloc->Reset();
		cmdContext.m_copyCmdList->Reset(cmdContext.m_copyCmdAlloc.Get(), nullptr);

		D3D12_RESOURCE_BARRIER copyBarrier{};
		CreateResourcesTransitionBarrier(copyBarrier, staging.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		cmdContext.m_copyCmdList->ResourceBarrier(1, &copyBarrier);

		cmdContext.m_copyCmdList->CopyResource(matBuffer, staging.Get());

		cmdContext.m_copyCmdList->Close();
		ID3D12CommandList* lists[]{ cmdContext.m_copyCmdList.Get() };
		queue->ExecuteCommandLists(1, lists);

		PlaceFence(cmdContext.m_copyFence.m_value, queue, cmdContext.m_copyFence.m_dx12Handle.Get(), cmdContext.m_copyFence.m_event);

		return 1;
	}

	static void CreateResourceViews(ID3D12Device* device, DescriptorContext& ctx, CmdContext& cmdContext, ID3D12CommandQueue* queue, ReadOnlyResources& roResources, 
		ReadWriteResources* rwResourcesArray, BlitzenEngine::DrawContext& context, DX12WRAPPER<ID3D12Resource>* pDepthTargets, UINT drawWidth, UINT drawHeight)
	{
		const auto& vertices{ context.m_meshes.m_hlslVtxs };
		const auto& transforms{ context.m_renders.m_transforms };
		const auto& surfaces{ context.m_meshes.m_surfaces };
		auto pRenders{ context.m_renders.m_renders };
		const auto& lods{ context.m_meshes.m_LODs };
		auto pMaterials{ context.m_textures.m_materials };
		auto materialCount{ context.m_textures.m_materialCount };

		for (size_t i = 0; i < ce_framesInFlight; ++i)
		{
			// Saves the offset of this group of descriptors and begins
			ctx.m_opaqueDrawViewsExclusiveOffset[i] = ctx.m_viewHeapCurrentOffset;
			ctx.m_opaqueDrawViewsExclusiveHandle[i] = ctx.m_viewHeapHandle;
			ctx.m_opaqueDrawViewsExclusiveHandle[i].ptr += ctx.m_opaqueDrawViewsExclusiveOffset[i] * ctx.m_viewHeapIncrement;

			CreateBufferShaderResourceView(device, roResources.m_vtxBuffer.buffer.Get(), ctx, (UINT)vertices.GetSize(), sizeof(BlitzenEngine::Vertex));
		}

		// Teams of descriptors used by both graphics and compute pipelines
		for (size_t i = 0; i < ce_framesInFlight; ++i)
		{
			// Saves the offset of this group of descriptors and begins
			ctx.m_sharedViewsOffset[i] = ctx.m_viewHeapCurrentOffset;
			ctx.m_sharedViewHandle[i] = ctx.m_viewHeapHandle;
			ctx.m_sharedViewHandle[i].ptr += ctx.m_sharedViewsOffset[i] * ctx.m_viewHeapIncrement;

			auto& rwResources = rwResourcesArray[i];

			CreateBufferShaderResourceView(device, roResources.m_surfaceBuffer.buffer.Get(), ctx, (UINT)surfaces.GetSize(), sizeof(BlitzenEngine::PrimitiveSurface));

			CreateBufferShaderResourceView(device, rwResources.m_transformBuffer.buffer.Get(), ctx, context.m_renders.m_transformCount, sizeof(BlitzenEngine::MeshTransform));

			CreateBufferShaderResourceView(device, roResources.m_renderBuffer.buffer.Get(), ctx, context.m_renders.m_renderCount, sizeof(BlitzenEngine::RenderObject));

			CreateConstantBufferView(device, ctx, rwResources.m_viewBuffer.buffer.Get(), sizeof(BlitzenEngine::CameraViewData));
		}

		// Teams of descriptors used for draw culling shaders
		for (size_t i = 0; i < ce_framesInFlight; ++i)
		{
			// Saves the offset of this group of descriptors and begins
			ctx.m_drawCullViewsOffset[i] = ctx.m_viewHeapCurrentOffset;
			ctx.m_drawCullViewsHandle[i] = ctx.m_viewHeapHandle;
			ctx.m_drawCullViewsHandle[i].ptr += ctx.m_drawCullViewsOffset[i] * ctx.m_viewHeapIncrement;

			auto& rwResources = rwResourcesArray[i];

			CreateBufferUnorderedAccessView(device, ctx, rwResources.m_drawCmdBuffer.buffer.Get(), rwResources.m_drawCmdCounterBuffer.buffer.Get(), 
				Ce_IndirectDrawCmdBufferSize, sizeof(IndirectDrawCmd), 0);

			CreateBufferUnorderedAccessView(device, ctx, rwResources.m_drawCmdCounterBuffer.buffer.Get(), nullptr, 1, sizeof(uint32_t), 0);

			CreateBufferShaderResourceView(device, roResources.m_LODBuffer.buffer.Get(), ctx, (UINT)lods.GetSize(), sizeof(BlitzenEngine::LodData));	
		}

		// Instancing unique descriptors
		if (BlitzenCore::Ce_InstanceCulling)
		{
			for (uint32_t i = 0; i < ce_framesInFlight; ++i)
			{
				auto& rwResources = rwResourcesArray[i];

				ctx.m_drawCullInstUAVsOffset[i] = ctx.m_viewHeapCurrentOffset;
				ctx.m_drawCullInstUAVsHandle[i] = ctx.m_viewHeapHandle;
				ctx.m_drawCullInstUAVsHandle[i].ptr += ctx.m_drawCullInstUAVsOffset[i] * ctx.m_viewHeapIncrement;

				CreateBufferUnorderedAccessView(device, ctx, rwResources.m_drawInstBuffer.buffer.Get(), nullptr, UINT(lods.GetSize() * BlitzenCore::Ce_MaxInstanceCountPerLOD), 
					sizeof(uint32_t), 0);

				CreateBufferUnorderedAccessView(device, ctx, rwResources.m_instCounterBuffer.buffer.Get(), nullptr, (UINT)lods.GetSize(), sizeof(BlitzenEngine::LodInstanceCounter), 0);
			}
		}

		// Teams of descriptors used for occlusion shaders
		if constexpr (CE_DX12OCCLUSION)
		{
			for (uint32_t i = 0; i < ce_framesInFlight; ++i)
			{
				auto& rwResources = rwResourcesArray[i];

				ctx.m_drawVisUAVOffset[i] = ctx.m_viewHeapCurrentOffset;
				ctx.m_drawVisUANHandle[i] = ctx.m_viewHeapHandle;
				ctx.m_drawVisUANHandle[i].ptr += ctx.m_drawVisUAVOffset[i] * ctx.m_viewHeapIncrement;

				CreateBufferUnorderedAccessView(device, ctx, rwResources.m_drawVisBuffer.buffer.Get(), nullptr, context.m_renders.m_renderCount, sizeof(uint32_t), 0);
			}
		}

		if constexpr (CE_DX12_BUILD_HI_Z_MAP)
		{
			CreateDepthPyramidDescriptors(device, rwResourcesArray, ctx, pDepthTargets, drawWidth, drawHeight);
		}

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{

		}
		
		// material buffer, single srv bound to pixel shader
		ctx.m_materialSRVOffset = ctx.m_viewHeapCurrentOffset;
		ctx.m_materialSRVHandle = ctx.m_viewHeapHandle;
		ctx.m_materialSRVHandle.ptr += ctx.m_materialSRVOffset * ctx.m_viewHeapIncrement;
		CreateBufferShaderResourceView(device, roResources.m_matBuffer.buffer.Get(), ctx, (UINT)materialCount, sizeof(BlitzenEngine::Material));

		// TEXTURE DESCRIPTORS
		ctx.m_texDescriptorsSRVOffset = ctx.m_viewHeapCurrentOffset;
		ctx.m_texDescriptorsSRVHandle = ctx.m_viewHeapHandle;
		ctx.m_texDescriptorsSRVHandle.ptr += ctx.m_texDescriptorsSRVOffset * ctx.m_viewHeapIncrement;
		for (size_t i = 0; i < roResources.m_textureCount; ++i)
		{
			auto& tex2D{ roResources.m_drawTextures[i] };

			CreateTexture2DShaderResourceView(device, tex2D.resource.Get(), ctx, tex2D.format, tex2D.mipLevels);
		}
	}

	uint8_t Dx12Renderer::SetupForRendering(BlitzenEngine::DrawContext& context)
	{
		GenerateHlslVertices(context.m_meshes);

		if (!BlitzenEngine::GenerateHLSLClusters(context.m_meshes))
		{
			BLIT_ERROR("Failed to generate HLSL clusters");
			return 0;
		}

		// Texture sampler offsets before creation
		m_descriptorContext.m_texSmpOffset = m_descriptorContext.m_samplerHeapCurrentOffset;
		m_descriptorContext.m_texSmpHandle = m_descriptorContext.m_samplerHeapHandle;
		m_descriptorContext.m_texSmpHandle.ptr += m_descriptorContext.m_texSmpOffset * m_descriptorContext.m_samplerHeapIncrement;

		CreateSampler(m_device.Get(), m_descriptorContext, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, nullptr, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

		if (!CreateRootSignatures(m_device.Get(), m_pipelineContext))
		{
			BLIT_ERROR("Failed to create root signatures");
			return 0;
		}

		if (!CreateCmdSignatures(m_device.Get(), m_pipelineContext))
		{
			BLIT_ERROR("Failed to create command signatures");
			return 0;
		}

		if (!CreateConstBuffers(m_device.Get(), m_cmdContext[m_currentFrame], m_transferCommandQueue.Get(), context, m_roResources))
		{
			BLIT_ERROR("Failed to create constant buffers");
			return 0;
		}

		if (!CreateVarBuffers(m_device.Get(), m_transferCommandQueue.Get(), m_cmdContext[m_currentFrame], m_rwResources, context, m_swapchainWidth, m_swapchainHeight))
		{
			BLIT_ERROR("Failed to create var buffers");
			return 0;
		}

		CreateResourceViews(m_device.Get(), m_descriptorContext, m_cmdContext[m_currentFrame], m_transferCommandQueue.Get(),
			m_roResources, m_rwResources, context, m_depthBuffers, m_swapchainWidth, m_swapchainHeight);

		if (!CheckForDeviceRemoval(m_device.Get()))
		{
			BLIT_ERROR("Failed to create shader resource views");
			return 0;
		}

		if (!CreatePipelines(m_device.Get(), m_pipelineContext))
		{
			BLIT_ERROR("Failed to create graphics pipelines");
			return 0;
		}

		// Gives the pyramid size to th camera. There is not much reason for the camera to have it, but it is what it is.
		context.m_camera.viewData.pyramidWidth = float(m_rwResources[0].m_HI_Z.width);
		context.m_camera.viewData.pyramidHeight = float(m_rwResources[0].m_HI_Z.height);

		return 1;
	}

	void Dx12Renderer::FinalSetup()
	{
		auto& cmdContext{ m_cmdContext[m_currentFrame] };

		cmdContext.m_graphicsCmdAlloc->Reset();
		cmdContext.m_graphicsCmdList->Reset(cmdContext.m_graphicsCmdAlloc.Get(), nullptr);

		// READ ONLY BARRIERS
		D3D12_RESOURCE_BARRIER staticBufferBarriers[Ce_ConstDataSSBOCount]{};

		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_VertexStagingBufferIndex], m_roResources.m_vtxBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_IndexStagingBufferIndex], m_roResources.m_idxBuffer.m_buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_SurfaceStagingBufferIndex], m_roResources.m_surfaceBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_RenderStagingBufferIndex], m_roResources.m_renderBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_LodStagingIndex], m_roResources.m_LODBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_MaterialStagingIndex], m_roResources.m_matBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		// EXECUTE
		cmdContext.m_graphicsCmdList->ResourceBarrier(Ce_ConstDataSSBOCount, staticBufferBarriers);

		// RW BUFFERS
		uint32_t rwID{ 0 };
		BlitCL::DynamicArray<D3D12_RESOURCE_BARRIER> rwBuffersFinal{ Ce_VarBuffersCount };

		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			CreateResourcesTransitionBarrier(rwBuffersFinal[rwID], m_rwResources[i].m_viewBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ);
			rwID++;
		}

		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			// Starts off as indirect argument, because the first transition barrier will be expecting that
			CreateResourcesTransitionBarrier(rwBuffersFinal[rwID], m_rwResources[i].m_drawCmdBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			rwID++;
		}

		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			// Starts off as indirect argument, because the first transition barrier will be expecting that
			CreateResourcesTransitionBarrier(rwBuffersFinal[rwID], m_rwResources[i].m_drawCmdCounterBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			rwID++;
		}

		if constexpr (BlitzenCore::Ce_InstanceCulling)
		{
			for (uint32_t i = 0; i < ce_framesInFlight; ++i)
			{
				D3D12_RESOURCE_BARRIER drawInstBufferBarrier{};
				CreateResourcesTransitionBarrier(drawInstBufferBarrier, m_rwResources[i].m_drawInstBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				rwBuffersFinal.PushBack(drawInstBufferBarrier);
			}

			for (uint32_t i = 0; i < ce_framesInFlight; ++i)
			{
				D3D12_RESOURCE_BARRIER lodInstBufferBarrier{};
				CreateResourcesTransitionBarrier(lodInstBufferBarrier, m_rwResources[i].m_instCounterBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				rwBuffersFinal.PushBack(lodInstBufferBarrier);
			}
		}

		if constexpr (CE_DX12OCCLUSION)
		{
			
			for (uint32_t i = 0; i < ce_framesInFlight; ++i)
			{
				D3D12_RESOURCE_BARRIER drawVisibilityBarrier{};
				CreateResourcesTransitionBarrier(drawVisibilityBarrier, m_rwResources[i].m_drawVisBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				rwBuffersFinal.PushBack(drawVisibilityBarrier);
			}
		}

		if constexpr (CE_DX12_BUILD_HI_Z_MAP)
		{
			for (uint32_t f = 0; f < ce_framesInFlight; ++f)
			{
				for (uint32_t hi_z_mip = 0; hi_z_mip < m_rwResources[f].m_HI_Z.mipCount; ++hi_z_mip)
				{
					D3D12_RESOURCE_BARRIER depthPyramidBarrier{};
					CreateResourcesTransitionBarrier(depthPyramidBarrier, m_rwResources[f].m_HI_Z.pyramid.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, hi_z_mip);

					rwBuffersFinal.PushBack(depthPyramidBarrier);
				}
			}
		}

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{

		}

		// EXECUTE
		cmdContext.m_graphicsCmdList->ResourceBarrier((UINT)rwBuffersFinal.GetSize(), rwBuffersFinal.Data());

		// TEXTURES
		for (uint32_t i = 0; i < m_roResources.m_textureCount; ++i)
		{
			D3D12_RESOURCE_BARRIER textureFinalBarrier{};
			CreateResourcesTransitionBarrier(textureFinalBarrier, m_roResources.m_drawTextures[i].resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			cmdContext.m_graphicsCmdList->ResourceBarrier(1, &textureFinalBarrier);
		}

		cmdContext.m_graphicsCmdList->Close();
		ID3D12CommandList* commandLists[] = { cmdContext.m_graphicsCmdList.Get() };
		m_commandQueue->ExecuteCommandLists(1, commandLists);

		PlaceFence(cmdContext.m_frameFence.m_value, m_commandQueue.Get(), cmdContext.m_frameFence.m_dx12Handle.Get(), cmdContext.m_frameFence.m_event);
	}
}

#endif