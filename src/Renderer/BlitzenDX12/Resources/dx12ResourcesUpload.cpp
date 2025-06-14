#if defined(_WIN32)

#include "Renderer/BlitzenDX12/Context/dx12Renderer.h"
#include "Renderer/BlitzenDX12/Resources/dx12Pipelines.h"
#include "Renderer/BlitzenDX12/Resources/dx12Resources.h"
#include "Renderer/BlitzenDX12/Resources/dx12RNDResources.h"

namespace BlitzenDX12
{
	DXGI_FORMAT GetDDSFormat(const BlitzenEngine::DDS_HEADER& header, const BlitzenEngine::DDS_HEADER_DXT10& header10)
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
			{
				return (DXGI_FORMAT)header10.dxgiFormat;
			}
			}
		}

		return DXGI_FORMAT_UNKNOWN;
	}

	uint8_t LoadDDSImageData(BlitzenEngine::DDS_HEADER& header, BlitzenEngine::DDS_HEADER_DXT10& header10, BlitzenPlatform::C_FILE_SCOPE& scopedFILE, DXGI_FORMAT& format, void* pData, uint32_t& blockSize)
	{
		format = GetDDSFormat(header, header10);
		if (format == DXGI_FORMAT_UNKNOWN)
		{
			BLIT_ERROR("Unknow format retrieved from DDS image");
			return 0;
		}

		auto file = scopedFILE.m_pHandle;

		blockSize = BlitzenEngine::GetDDSBlockSize(header, header10);
		size_t imageSize = BlitzenEngine::GetDDSImageSizeBC(header.dwWidth, header.dwHeight, header.dwMipMapCount, blockSize);

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

	uint8_t Create2DTexture(ID3D12Device* device, DX12WRAPPER<ID3D12Resource>& resource, UINT width, UINT height, UINT mipLevels,
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

	uint8_t UploadResourcesToBuffers(ID3D12Device* device, const BlitzenEngine::DrawContext& drawContext, ReadOnlyResources& roResources, ReadWriteResources* rwResourcesArr, 
		CmdContext& cmdContext, ID3D12CommandQueue* commandQueue)
	{
		CPU_DATA_SSBO_SIZE_INFO transformBufferSizeInfo{};
		transformBufferSizeInfo.m_fullSSBOSize = BlitzenCore::Ce_MaxWorldTransformCount;
		transformBufferSizeInfo.m_staticDataSize = drawContext.m_pResidents->m_transforms.m_staticTransformCount;
		transformBufferSizeInfo.m_staticDataOffset = BlitzenEngine::CE_STATIC_TRANSFORM_OFFSET;
		transformBufferSizeInfo.m_dynamicDataSize = drawContext.m_pResidents->m_transforms.m_dynamicTransformCount;
		transformBufferSizeInfo.m_dynamicDataOffset = BlitzenEngine::CE_DYNAMIC_TRANSFORM_OFFSET;

		for (UINT frame = 0; frame < ce_framesInFlight; ++frame)
		{
			auto& rwResources{ rwResourcesArr[frame] };

			STAGING<BlitzenEngine::MeshTransform> staticTransformStaging;
			if (!CreateCPU_WRITE_SSBO_Stagings(device, staticTransformStaging, rwResources.m_transformBuffer.m_dynamicDataStaging, drawContext.m_pResidents->m_transforms.m_transforms,
				transformBufferSizeInfo))
			{
				BLIT_ERROR("%s: Failed to create transform staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			STAGING<uint32_t> drawVisibilityStaging{ };
			// DRAW OCC MODE
			if constexpr (BlitzenCore::Ce_OcclusionCulling)
			{
				BlitCL::DynamicArray<uint32_t> zeroData{ drawContext.m_pResidents->m_renders.m_renderCount, 0 };

				// Normally only needed for non-temporal occlusion, but right now it gets created anyway, which is a bit of a waste
				if(!CreateStaging(device, drawVisibilityStaging, drawContext.m_pResidents->m_renders.m_renderCount, zeroData.Data()))
				{
					BLIT_ERROR("%s: Failed to create visibility staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
					return 0;
				}
			}

			STAGING<BlitzenEngine::LodInstanceCounter> lodInstStaging{ nullptr };
			// DRAW CULL INST MODE
			if constexpr (BlitzenCore::Ce_InstanceCulling)
			{
				if (!CreateStaging(device, lodInstStaging, drawContext.m_meshes.m_lodInstanceList.GetSize(), drawContext.m_meshes.m_lodInstanceList.Data()))
				{
					BLIT_ERROR("%s: Failed to create lod instance counting staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
					return 0;
				}
			}

			cmdContext.m_copyCmdAlloc->Reset();
			cmdContext.m_copyCmdList->Reset(cmdContext.m_copyCmdAlloc.Get(), nullptr);

			// DEST BARRIERS
			BlitCL::DynamicArray<D3D12_RESOURCE_BARRIER> copyDestBarriers{ Ce_VarSSBODataCount };

			CreateResourcesTransitionBarrier(copyDestBarriers[0], rwResources.m_transformBuffer.m_ssbo.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

			// DRAW OCC MODE VIS BUFFER (normally not needed for temporal occlusion)
			if constexpr (BlitzenCore::Ce_OcclusionCulling)
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

			CreateResourcesTransitionBarrier(copySourceBarriers[0], staticTransformStaging.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

			if constexpr (BlitzenCore::Ce_OcclusionCulling)
			{
				D3D12_RESOURCE_BARRIER visibilityBufferSourceBarrier{};
				CreateResourcesTransitionBarrier(visibilityBufferSourceBarrier, drawVisibilityStaging.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
				copySourceBarriers.PushBack(visibilityBufferSourceBarrier);
			}

			if constexpr (BlitzenCore::Ce_InstanceCulling)
			{
				D3D12_RESOURCE_BARRIER lodInstBufferSourceBarrier{};
				CreateResourcesTransitionBarrier(lodInstBufferSourceBarrier, lodInstStaging.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
				copySourceBarriers.PushBack(lodInstBufferSourceBarrier);
			}

			// Execute
			cmdContext.m_copyCmdList->ResourceBarrier(UINT(copySourceBarriers.GetSize()), copySourceBarriers.Data());

			// visibilities zeroed
			if constexpr (BlitzenCore::Ce_OcclusionCulling)
			{
				cmdContext.m_copyCmdList->CopyBufferRegion(rwResources.m_drawVisBuffer.buffer.Get(), 0,  drawVisibilityStaging.m_buffer.Get(), 0, drawVisibilityStaging.m_dataSize);
			}

			// instance counter
			if constexpr (BlitzenCore::Ce_InstanceCulling)
			{
				cmdContext.m_copyCmdList->CopyBufferRegion(rwResources.m_instCounterBuffer.buffer.Get(), 0, lodInstStaging.m_buffer.Get(), 0, lodInstStaging.m_dataSize);
			}

			// Puts persistent transform staging in copy source state forever
			D3D12_RESOURCE_BARRIER dynamicTransformBarrier{};
			CreateResourcesTransitionBarrier(dynamicTransformBarrier, rwResources.m_transformBuffer.m_dynamicDataStaging.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
			// execute
			cmdContext.m_copyCmdList->ResourceBarrier(1, &dynamicTransformBarrier);

			cmdContext.m_copyCmdList->CopyBufferRegion(rwResources.m_transformBuffer.m_ssbo.buffer.Get(), BlitzenEngine::CE_DYNAMIC_TRANSFORM_OFFSET * sizeof(BlitzenEngine::MeshTransform), 
				rwResources.m_transformBuffer.m_dynamicDataStaging.m_buffer.Get(), 0, rwResources.m_transformBuffer.m_dynamicDataStaging.m_dataSize);

			cmdContext.m_copyCmdList->CopyBufferRegion(rwResources.m_transformBuffer.m_ssbo.buffer.Get(), BlitzenEngine::CE_STATIC_TRANSFORM_OFFSET * sizeof(BlitzenEngine::MeshTransform),
				staticTransformStaging.m_buffer.Get(), 0, staticTransformStaging.m_dataSize);

			cmdContext.m_copyCmdList->Close();
			ID3D12CommandList* commandLists[] = { cmdContext.m_copyCmdList.Get() };
			commandQueue->ExecuteCommandLists(1, commandLists);

			PlaceFence(cmdContext.m_copyFence.m_value, commandQueue, cmdContext.m_copyFence.m_dx12Handle.Get(), cmdContext.m_copyFence.m_event);
		}

		const auto& vertices{ drawContext.m_meshes.m_hlslVtxs };
		const auto& indices{ BlitzenCore::Ce_BuildClusters ? drawContext.m_meshes.m_clusterIndices : drawContext.m_meshes.m_indices };
		const auto& surfaces{ drawContext.m_meshes.m_surfaces };
		const auto& lods{ drawContext.m_meshes.m_LODs };

		STAGING<BlitzenEngine::HlslVtx> vertexStagingBuffer{ nullptr };
		if (!CreateStaging(device, vertexStagingBuffer, vertices.GetSize(), vertices.Data()))
		{
			BLIT_ERROR("%s: Failed to create vertex staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		DX12WRAPPER<ID3D12Resource> indexStagingBuffer{ nullptr };
		UINT64 idxBufferSize{ sizeof(uint32_t) * drawContext.m_meshes.m_indices.GetSize() };

		if (!CreateBuffer(device, indexStagingBuffer.ReleaseAndGetAddressOf(), idxBufferSize, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_UPLOAD))
		{
			BLIT_ERROR("%s: Failed to create index staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		void* pMappedData{ nullptr };
		HRESULT mappingRes{ indexStagingBuffer->Map(0, nullptr, &pMappedData) };
		if (FAILED(mappingRes))
		{
			BLIT_ERROR("%s: Failed to map pointer to index staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return LOG_ERROR_MESSAGE_AND_RETURN(mappingRes);
		}

		BlitzenCore::BlitMemCopy(pMappedData, drawContext.m_meshes.m_indices.Data(), idxBufferSize);

		STAGING<BlitzenEngine::PrimitiveSurface> surfaceStagingBuffer{ };
		if (!CreateStaging(device, surfaceStagingBuffer, surfaces.GetSize(), surfaces.Data()))
		{
			BLIT_ERROR("Failed to create surface buffer");
			return 0;
		}

		STAGING<BlitzenEngine::RenderObject> renderStagingBuffer{ nullptr };
		if (!CreateStaging(device, renderStagingBuffer, drawContext.m_pResidents->m_renders.m_renderCount, drawContext.m_pResidents->m_renders.m_renders))
		{
			BLIT_ERROR("%s: Failed to create render staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		STAGING<BlitzenEngine::LodData> lodStaging{ nullptr };
		if (!CreateStaging(device, lodStaging, lods.GetSize(), lods.Data()))
		{
			BLIT_ERROR("%s: Failed to create LOD staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		STAGING<BlitzenEngine::Material> materialStaging{ nullptr };
		if (!CreateStaging(device, materialStaging, drawContext.m_textures.m_materialCount, drawContext.m_textures.m_materials))
		{
			BLIT_ERROR("%s: Failed to create material staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		STAGING<BlitzenEngine::HCluster> clusterStaging{ nullptr };
		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			if (!CreateStaging(device, clusterStaging, drawContext.m_meshes.m_hlslClusterCount, drawContext.m_meshes.m_hlslClusters))
			{
				BLIT_ERROR("%s: Failed to create cluster staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}
		}

		// DATA COPIES
		cmdContext.m_copyCmdAlloc->Reset();
		cmdContext.m_copyCmdList->Reset(cmdContext.m_copyCmdAlloc.Get(), nullptr);

		// DEST BARRIERS
		BlitCL::DynamicArray<D3D12_RESOURCE_BARRIER> copyDestBarriers{ Ce_ConstDataSSBOCount, {} };

		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_VertexStagingBufferIndex], roResources.m_vtxBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_IndexStagingBufferIndex], roResources.m_idxBuffer.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_SurfaceStagingBufferIndex], roResources.m_surfaceBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_RenderStagingBufferIndex], roResources.m_renderBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_LodStagingIndex], roResources.m_LODBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_MaterialStagingIndex], roResources.m_matBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			D3D12_RESOURCE_BARRIER clusterBufferBarrier{};
			CreateResourcesTransitionBarrier(clusterBufferBarrier, roResources.m_clusterBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

			copyDestBarriers.PushBack(clusterBufferBarrier);
		}

		// execute
		roResources.BUFFER_COUNT = (UINT(copyDestBarriers.GetSize()));
		cmdContext.m_copyCmdList->ResourceBarrier(roResources.BUFFER_COUNT, copyDestBarriers.Data());

		// SRC BARRIERS
		BlitCL::DynamicArray<D3D12_RESOURCE_BARRIER> copySourceBarriers{ Ce_ConstDataSSBOCount, {} };

		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_VertexStagingBufferIndex], vertexStagingBuffer.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_IndexStagingBufferIndex], indexStagingBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_SurfaceStagingBufferIndex], surfaceStagingBuffer.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_RenderStagingBufferIndex], renderStagingBuffer.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_LodStagingIndex], lodStaging.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_MaterialStagingIndex], materialStaging.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			D3D12_RESOURCE_BARRIER clusterStagingBarrier{};
			CreateResourcesTransitionBarrier(clusterStagingBarrier, clusterStaging.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

			copySourceBarriers.PushBack(clusterStagingBarrier);
		}

		if (copySourceBarriers.GetSize() != roResources.BUFFER_COUNT)
		{
			BLIT_ERROR("SRC and DEST read only buffer count difference");
			return 0;
		}

		// execute
		cmdContext.m_copyCmdList->ResourceBarrier(Ce_ConstDataSSBOCount, copySourceBarriers.Data());

		cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_vtxBuffer.buffer.Get(), 0, vertexStagingBuffer.m_buffer.Get(), 0, vertexStagingBuffer.m_dataSize);
		cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_idxBuffer.m_buffer.Get(), 0, indexStagingBuffer.Get(), 0, idxBufferSize);
		cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_surfaceBuffer.buffer.Get(), 0, surfaceStagingBuffer.m_buffer.Get(), 0, surfaceStagingBuffer.m_dataSize);
		cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_renderBuffer.buffer.Get(), 0, renderStagingBuffer.m_buffer.Get(), 0, renderStagingBuffer.m_dataSize);
		cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_LODBuffer.buffer.Get(), 0, lodStaging.m_buffer.Get(), 0, lodStaging.m_dataSize);
		cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_matBuffer.buffer.Get(), 0, materialStaging.m_buffer.Get(), 0, materialStaging.m_dataSize);
		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			cmdContext.m_copyCmdList->CopyResource(roResources.m_clusterBuffer.buffer.Get(), clusterStaging.m_buffer.Get());
		}

		cmdContext.m_copyCmdList->Close();
		ID3D12CommandList* commandLists[] = { cmdContext.m_copyCmdList.Get() };
		commandQueue->ExecuteCommandLists(1, commandLists);

		PlaceFence(cmdContext.m_copyFence.m_value, commandQueue, cmdContext.m_copyFence.m_dx12Handle.Get(), cmdContext.m_copyFence.m_event);

		// success
		return 1;
	}

	void CreateResourceViews(ID3D12Device* device, DescriptorContext& ctx, CmdContext& cmdContext, ID3D12CommandQueue* queue, ReadOnlyResources& roResources, 
		ReadWriteResources* rwResourcesArray, BlitzenEngine::DrawContext& context, DX12WRAPPER<ID3D12Resource>* pDepthTargets, UINT drawWidth, UINT drawHeight)
	{
		const auto& vertices{ context.m_meshes.m_hlslVtxs };
		const auto& surfaces{ context.m_meshes.m_surfaces };
		const auto& lods{ context.m_meshes.m_LODs };

		// DRAW DESCRIPTORS
		for (size_t i = 0; i < ce_framesInFlight; ++i)
		{
			ctx.m_opaqueDrawViewsExclusiveOffset[i] = ctx.m_viewHeapCurrentOffset;
			ctx.m_opaqueDrawViewsExclusiveHandle[i] = ctx.m_viewHeapHandle;
			ctx.m_opaqueDrawViewsExclusiveHandle[i].ptr += ctx.m_opaqueDrawViewsExclusiveOffset[i] * ctx.m_viewHeapIncrement;

			CreateBufferShaderResourceView(device, roResources.m_vtxBuffer.buffer.Get(), ctx, (UINT)vertices.GetSize(), sizeof(BlitzenEngine::Vertex));
		}

		// SHARED DESCRIPTORS
		for (size_t i = 0; i < ce_framesInFlight; ++i)
		{
			ctx.m_sharedViewsOffset[i] = ctx.m_viewHeapCurrentOffset;
			ctx.m_sharedViewHandle[i] = ctx.m_viewHeapHandle;
			ctx.m_sharedViewHandle[i].ptr += ctx.m_sharedViewsOffset[i] * ctx.m_viewHeapIncrement;

			auto& rwResources = rwResourcesArray[i];

			CreateBufferShaderResourceView(device, roResources.m_surfaceBuffer.buffer.Get(), ctx, (UINT)surfaces.GetSize(), sizeof(BlitzenEngine::PrimitiveSurface));

			CreateBufferShaderResourceView(device, rwResources.m_transformBuffer.m_ssbo.buffer.Get(), ctx, BlitzenCore::Ce_MaxDynamicObjectCount + context.m_pResidents->m_transforms.m_staticTransformCount, 
				sizeof(BlitzenEngine::MeshTransform));

			CreateBufferShaderResourceView(device, roResources.m_renderBuffer.buffer.Get(), ctx, context.m_pResidents->m_renders.m_renderCount, sizeof(BlitzenEngine::RenderObject));

			CreateConstantBufferView(device, ctx, rwResources.m_viewBuffer.buffer.Get(), sizeof(BlitzenEngine::CameraViewData));
		}

		// CULLING DESCRIPTORS (all modes)
		for (size_t i = 0; i < ce_framesInFlight; ++i)
		{
			ctx.m_drawCullViewsOffset[i] = ctx.m_viewHeapCurrentOffset;
			ctx.m_drawCullViewsHandle[i] = ctx.m_viewHeapHandle;
			ctx.m_drawCullViewsHandle[i].ptr += ctx.m_drawCullViewsOffset[i] * ctx.m_viewHeapIncrement;

			auto& rwResources = rwResourcesArray[i];

			CreateBufferUnorderedAccessView(device, ctx, rwResources.m_drawCmdBuffer.buffer.Get(), rwResources.m_drawCmdCounterBuffer.buffer.Get(), 
				Ce_IndirectDrawCmdBufferSize, sizeof(IndirectDrawCmd), 0);

			CreateBufferUnorderedAccessView(device, ctx, rwResources.m_drawCmdCounterBuffer.buffer.Get(), nullptr, 1, sizeof(uint32_t), 0);

			CreateBufferShaderResourceView(device, roResources.m_LODBuffer.buffer.Get(), ctx, (UINT)lods.GetSize(), sizeof(BlitzenEngine::LodData));	
		}

		// INSTANCING DESCRIPTORS
		if (BlitzenCore::Ce_InstanceCulling)
		{
			for (uint32_t i = 0; i < ce_framesInFlight; ++i)
			{
				auto& rwResources = rwResourcesArray[i];

				ctx.m_drawCullInstUAVsOffset[i] = ctx.m_viewHeapCurrentOffset;
				ctx.m_drawCullInstUAVsHandle[i] = ctx.m_viewHeapHandle;
				ctx.m_drawCullInstUAVsHandle[i].ptr += ctx.m_drawCullInstUAVsOffset[i] * ctx.m_viewHeapIncrement;

				CreateBufferUnorderedAccessView(device, ctx, rwResources.m_drawInstBuffer.buffer.Get(), nullptr, UINT(lods.GetSize() * BlitzenEngine::CE_MAX_INSTANCES_PER_LOD), 
					sizeof(uint32_t), 0);

				CreateBufferUnorderedAccessView(device, ctx, rwResources.m_instCounterBuffer.buffer.Get(), nullptr, (UINT)lods.GetSize(), sizeof(BlitzenEngine::LodInstanceCounter), 0);
			}
		}

		// VISIBILITY BUFFER FOR OCCLUSION (non-temporal)
		if constexpr (BlitzenCore::Ce_OcclusionCulling)
		{
			for (uint32_t i = 0; i < ce_framesInFlight; ++i)
			{
				auto& rwResources = rwResourcesArray[i];

				ctx.m_drawVisUAVOffset[i] = ctx.m_viewHeapCurrentOffset;
				ctx.m_drawVisUANHandle[i] = ctx.m_viewHeapHandle;
				ctx.m_drawVisUANHandle[i].ptr += ctx.m_drawVisUAVOffset[i] * ctx.m_viewHeapIncrement;

				CreateBufferUnorderedAccessView(device, ctx, rwResources.m_drawVisBuffer.buffer.Get(), nullptr, context.m_pResidents->m_renders.m_renderCount, sizeof(uint32_t), 0);
			}
		}

		// HI_Z_MAP DESCRIPTORS
		if constexpr (CE_DX12_BUILD_HI_Z_MAP)
		{
			CreateDepthPyramidDescriptors(device, rwResourcesArray, ctx, pDepthTargets, drawWidth, drawHeight);
		}

		// CLUSTER DESCRIPTORS
		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			for (uint32_t i = 0; i < ce_framesInFlight; ++i)
			{
				auto& rwResources{ rwResourcesArray[i] };

				ctx.m_clusterDispatchAdditionalUAVsOffset[i] = ctx.m_viewHeapCurrentOffset;
				ctx.m_clusterDispatchAdditionalUAVsHandle[i] = ctx.m_viewHeapHandle;
				ctx.m_clusterDispatchAdditionalUAVsHandle[i].ptr += ctx.m_clusterDispatchAdditionalUAVsOffset[i] * ctx.m_viewHeapIncrement;

				CreateBufferUnorderedAccessView(device, ctx, rwResources.m_clusterDispatchBuffer.buffer.Get(), nullptr, 1, sizeof(ClusterDispatchCmd), 0);

				CreateBufferUnorderedAccessView(device, ctx, rwResources.m_clusterVisibilityBuffer.buffer.Get(), nullptr, 1, Ce_ClusterGroupDataBufferSize * 64 * sizeof(uint32_t), 0);

				CreateBufferUnorderedAccessView(device, ctx, rwResources.m_clusterGroupDataBuffer.buffer.Get(), nullptr, Ce_ClusterGroupDataBufferSize, sizeof(ClusterGroupData), 0);

				CreateBufferShaderResourceView(device, roResources.m_clusterBuffer.buffer.Get(), ctx, context.m_meshes.m_hlslClusterCount, sizeof(BlitzenEngine::HCluster));
			}
		}
		
		// MAT DESCRIPTOR (pixel shader)
		ctx.m_materialSRVOffset = ctx.m_viewHeapCurrentOffset;
		ctx.m_materialSRVHandle = ctx.m_viewHeapHandle;
		ctx.m_materialSRVHandle.ptr += ctx.m_materialSRVOffset * ctx.m_viewHeapIncrement;
		CreateBufferShaderResourceView(device, roResources.m_matBuffer.buffer.Get(), ctx, context.m_textures.m_materialCount, sizeof(BlitzenEngine::Material));

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

	void Dx12Renderer::FinalSetup()
	{
		auto& cmdContext{ m_cmdContext[m_currentFrame] };

		cmdContext.m_graphicsCmdAlloc->Reset();
		cmdContext.m_graphicsCmdList->Reset(cmdContext.m_graphicsCmdAlloc.Get(), nullptr);

		// READ ONLY BARRIERS
		BlitCL::DynamicArray<D3D12_RESOURCE_BARRIER> staticBufferBarriers{ m_roResources.BUFFER_COUNT, {} };

		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_VertexStagingBufferIndex], m_roResources.m_vtxBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_IndexStagingBufferIndex], m_roResources.m_idxBuffer.m_buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_SurfaceStagingBufferIndex], m_roResources.m_surfaceBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_RenderStagingBufferIndex], m_roResources.m_renderBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_LodStagingIndex], m_roResources.m_LODBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_MaterialStagingIndex], m_roResources.m_matBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		if (BlitzenCore::Ce_BuildClusters)
		{
			CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_ClusterStagingIndex], m_roResources.m_clusterBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}

		// EXECUTE
		cmdContext.m_graphicsCmdList->ResourceBarrier(m_roResources.BUFFER_COUNT, staticBufferBarriers.Data());

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

		if constexpr (BlitzenCore::Ce_OcclusionCulling)
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
			for (uint32_t frame = 0; frame < ce_framesInFlight; ++frame)
			{
				auto& rwResources{ m_rwResources[frame] };

				D3D12_RESOURCE_BARRIER clusterDispatchBarrier{};
				CreateResourcesTransitionBarrier(clusterDispatchBarrier, rwResources.m_clusterDispatchBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

				rwBuffersFinal.PushBack(clusterDispatchBarrier);

				D3D12_RESOURCE_BARRIER clusterDispatchCounterBarrier{};
				CreateResourcesTransitionBarrier(clusterDispatchCounterBarrier, rwResources.m_clusterVisibilityBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				rwBuffersFinal.PushBack(clusterDispatchCounterBarrier);

				D3D12_RESOURCE_BARRIER clusterGroupDataBarrier{};
				CreateResourcesTransitionBarrier(clusterGroupDataBarrier, rwResources.m_clusterGroupDataBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				rwBuffersFinal.PushBack(clusterGroupDataBarrier);
			}
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