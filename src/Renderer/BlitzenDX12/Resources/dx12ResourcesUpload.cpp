#if defined(_WIN32)
#include "Renderer/BlitzenDX12/Context/dx12Renderer.h"
#include "Renderer/BlitzenDX12/Resources/dx12Pipelines.h"
#include "Renderer/BlitzenDX12/Resources/dx12Resources.h"
#include "Renderer/BlitzenDX12/Resources/dx12RNDResources.h"
#include "BlitCL/blitDynamicArr.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenDX12
{
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
			src.PlacedFootprint.Footprint.Width = (mipWidth + 3) / 4 * 4;  // Round up to 4-byte alignment
			src.PlacedFootprint.Footprint.Height = (mipHeight + 3) / 4 * 4;  // Same for height
			src.PlacedFootprint.Footprint.Depth = 1;
			src.PlacedFootprint.Footprint.RowPitch = ((mipWidth + 3) / 4) * blockSize;

			// Define the copy region (size of the mip level)
			D3D12_BOX box{};
			box.left = 0;
			box.top = 0;
			box.front = 0;
			box.right = src.PlacedFootprint.Footprint.Width;
			box.bottom = src.PlacedFootprint.Footprint.Height;
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

	uint8_t Dx12Renderer::UploadTexture(BLIT_STRAIGHTHANDLE pTexture, BlitzenEngine::DDS_HEADER& header, BlitzenEngine::DDS_HEADER_DXT10& header10, size_t imageSize, UINT blockSize, DXGI_FORMAT format)
	{
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
		tex2D.format = format;

		BlitzenCore::BlitMemCopy(pData, pTexture, imageSize);

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
		CPU_LOGIC_BUFFERS& cpuLogicBuffers, CmdContext& cmdContext, ID3D12CommandQueue* commandQueue, LoadingContextMesh& loadingContextMesh)
	{

		/******************************************************************************************************
		*	READ WRITE RESOURCES (RESOURCES WITH MULTIPLE COPIES OF THE BUFFER, HENCE THE LOOP)				  *
		*******************************************************************************************************/
		for (UINT frame = 0; frame < ce_framesInFlight; ++frame)
		{
			auto& rwResources{ rwResourcesArr[frame] };

			STAGING<BlitzenEngine::MeshTransform> transformStaging;
			if (!CreateStaging(device, transformStaging, BLIT_MAX_WORLD_TRANSFORM_COUNT, 
				drawContext.m_pResidents->m_transforms.m_transforms))
			{
				BLIT_ERROR("%s: Failed to create transform staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			// DRAW OCC MODE
			STAGING<uint32_t> drawVisibilityStaging{};
			if constexpr (BlitzenCore::Ce_OcclusionCulling)
			{
				BlitCL::DynamicArray<uint32_t> zeroData{ drawContext.m_pResidents->m_renders.RENDER_COUNT, 0 };

				// Normally only needed for non-temporal occlusion, but right now it gets created anyway, which is a bit of a waste
				if(!CreateStaging(device, drawVisibilityStaging, drawContext.m_pResidents->m_renders.RENDER_COUNT, zeroData.Data()))
				{
					BLIT_ERROR("%s: Failed to create visibility staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
					return 0;
				}
			}

			// DRAW CULL INST MODE
			if constexpr (BlitzenCore::Ce_InstanceCulling)
			{
				
			}

			STAGING<ClusterDispatchCmd> clusterDispatchStaging{};
			if constexpr (BlitzenCore::Ce_BuildClusters)
			{
				ClusterDispatchCmd yzOne{};
				yzOne.command.ThreadGroupCountX = 0;
				yzOne.command.ThreadGroupCountY = 1;
				yzOne.command.ThreadGroupCountZ = 1;
				if (!CreateStaging(device, clusterDispatchStaging, 1, &yzOne))
				{
					BLIT_ERROR("%s: Failed to create clustser dispatch staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
					return 0;
				}
			}

			cmdContext.m_copyCmdAlloc->Reset();
			cmdContext.m_copyCmdList->Reset(cmdContext.m_copyCmdAlloc.Get(), nullptr);

			// DEST BARRIERS
			BlitCL::DynamicArray<D3D12_RESOURCE_BARRIER> copyDestBarriers{ Ce_VarSSBODataCount };

			CreateResourcesTransitionBarrier(copyDestBarriers[0], rwResources.m_transformBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

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
				
			}

			if constexpr (BlitzenCore::Ce_BuildClusters)
			{
				D3D12_RESOURCE_BARRIER clusterDipatchBarrier{};
				CreateResourcesTransitionBarrier(clusterDipatchBarrier, rwResources.m_clusterGroupCounter.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

				copyDestBarriers.PushBack(clusterDipatchBarrier);
			}

			// Execute
			cmdContext.m_copyCmdList->ResourceBarrier((UINT)copyDestBarriers.GetSize(), copyDestBarriers.Data());

			// SRC BARRIERS
			BlitCL::DynamicArray<D3D12_RESOURCE_BARRIER> copySourceBarriers{ Ce_VarSSBODataCount };

			CreateResourcesTransitionBarrier(copySourceBarriers[0], transformStaging.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

			if constexpr (BlitzenCore::Ce_OcclusionCulling)
			{
				D3D12_RESOURCE_BARRIER visibilityBufferSourceBarrier{};
				CreateResourcesTransitionBarrier(visibilityBufferSourceBarrier, drawVisibilityStaging.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
				copySourceBarriers.PushBack(visibilityBufferSourceBarrier);
			}

			if constexpr (BlitzenCore::Ce_InstanceCulling)
			{
				
			}

			if constexpr (BlitzenCore::Ce_BuildClusters)
			{
				D3D12_RESOURCE_BARRIER clusterDispatchStagingBarrier{};
				CreateResourcesTransitionBarrier(clusterDispatchStagingBarrier, clusterDispatchStaging.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
			}

			// Execute
			cmdContext.m_copyCmdList->ResourceBarrier(UINT(copySourceBarriers.GetSize()), copySourceBarriers.Data());

			// visibilities zeroed
			if constexpr (BlitzenCore::Ce_OcclusionCulling)
			{
				cmdContext.m_copyCmdList->CopyBufferRegion(rwResources.m_drawVisBuffer.buffer.Get(), 0,  drawVisibilityStaging.m_buffer.Get(), 0, drawVisibilityStaging.m_dataSize);
			}

			if constexpr (BlitzenCore::Ce_InstanceCulling)
			{
				
			}

			if constexpr (BlitzenCore::Ce_BuildClusters)
			{
				cmdContext.m_copyCmdList->CopyBufferRegion(rwResources.m_clusterGroupCounter.buffer.Get(), 0, clusterDispatchStaging.m_buffer.Get(), 0, clusterDispatchStaging.m_dataSize);
			}

			cmdContext.m_copyCmdList->CopyBufferRegion(rwResources.m_transformBuffer.buffer.Get(), 0, transformStaging.m_buffer.Get(), 0, transformStaging.m_dataSize);

			cmdContext.m_copyCmdList->Close();
			ID3D12CommandList* commandLists[] = { cmdContext.m_copyCmdList.Get() };
			commandQueue->ExecuteCommandLists(1, commandLists);

			PlaceFence(cmdContext.m_copyFence.m_value, commandQueue, cmdContext.m_copyFence.m_dx12Handle.Get(), cmdContext.m_copyFence.m_event);
		}

		/******************************************************************************************************
		*	READ ONLY RESOURCES																				  *
		*******************************************************************************************************/

		STAGING<BlitzenEngine::VtxPos> terrainVtxPosStagingBuffer{};
		if (!CreateStaging(device, terrainVtxPosStagingBuffer, drawContext.m_pTerrain->terrainVertexCount, drawContext.m_pTerrain->terrainVertices))
		{
			BLIT_ERROR("%s: Failed to create terrain vertex staging Buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		STAGING<uint32_t> terrainVtxIdxStagingBuffer{};
		if (!CreateStaging(device, terrainVtxIdxStagingBuffer, drawContext.m_pTerrain->terrainIndexCount, drawContext.m_pTerrain->terrainIndices))
		{
			BLIT_ERROR("%s: Failed to create terrain index buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		STAGING<float> terrainHeightStaging{};
		if (!CreateStaging(device, terrainHeightStaging, drawContext.m_pTerrain->m_heightDataCount, drawContext.m_pTerrain->m_heightBufferData))
		{
			BLIT_ERROR("%s: Filaed to create terrain height buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		STAGING<BlitzenEngine::PrimitiveSurface> surfaceStagingBuffer{ };
		if (!CreateStaging(device, surfaceStagingBuffer, drawContext.m_meshes.m_meshPrimitives.m_meshPrimitivesCount, drawContext.m_meshes.m_meshPrimitives.m_meshPrimitives))
		{
			BLIT_ERROR("%s: Failed to create surface buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		STAGING<BlitzenEngine::RenderObject> renderStaging{ nullptr };
		if (!CreateStaging(device, renderStaging, BLIT_MAX_WORLD_RENDERS, drawContext.m_pResidents->m_renders.m_renders))
		{
			BLIT_ERROR("%s: Failed to create render staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		STAGING<BlitzenEngine::BoundingSphere> boundingSphereStaging{ nullptr };
		if (!CreateStaging(device, boundingSphereStaging, BLIT_MAX_WORLD_RENDERS, drawContext.m_pResidents->m_colliders.m_boundingSpheres))
		{
			BLIT_ERROR("%s: Failed to create bounding sphere buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		STAGING<BlitzenEngine::LodData> lodStaging{ nullptr };
		if (!CreateStaging(device, lodStaging, drawContext.m_meshes.m_meshPrimitives.m_LODCount, drawContext.m_meshes.m_meshPrimitives.m_LODs))
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

		STAGING<BlitzenEngine::WVTransform> worldVariableTransformStaging{ nullptr };
		if (!CreateStaging(device, worldVariableTransformStaging, drawContext.m_pResidents->m_transforms.m_moveableCount, drawContext.m_pResidents->m_transforms.WVWithMovement))
		{
			BLIT_ERROR("%s: Failed to create world variable transform staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		STAGING<BlitzenEngine::ClusterVertices> clusterVtxStaging{ nullptr };
		STAGING<BlitzenEngine::ClusterSphere> clusterSpheresStaging{ nullptr };
		STAGING<BlitzenEngine::ClusterCone> clusterConesStaging{ nullptr };
		STAGING<uint32_t> clusterIdxStaging{ nullptr };
		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			if (!CreateStaging(device, clusterVtxStaging, drawContext.m_meshes.m_clusters.m_clusterCount, drawContext.m_meshes.m_clusters.m_clusterVertices))
			{
				BLIT_ERROR("%s: Failed to create cluster vertices staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!CreateStaging(device, clusterSpheresStaging, drawContext.m_meshes.m_clusters.m_clusterCount, drawContext.m_meshes.m_clusters.m_clusterSpheres))
			{
				BLIT_ERROR("%s: Failed to create cluster spheres staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!CreateStaging(device, clusterConesStaging, drawContext.m_meshes.m_clusters.m_clusterCount, drawContext.m_meshes.m_clusters.m_clusterCones))
			{
				BLIT_ERROR("%s: Failed to create cluster cones staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!CreateStaging(device, clusterIdxStaging, drawContext.m_meshes.m_clusters.m_clusterIndicesCount, drawContext.m_meshes.m_clusters.m_clusterIndices))
			{
				BLIT_ERROR("%s: Failed to create cluster indices staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}
		}

		// DATA COPIES
		cmdContext.m_copyCmdAlloc->Reset();
		cmdContext.m_copyCmdList->Reset(cmdContext.m_copyCmdAlloc.Get(), nullptr);

		// DEST BARRIERS
		BlitCL::DynamicArray<D3D12_RESOURCE_BARRIER> copyDestBarriers{ Ce_ConstDataSSBOCount, {} };

		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_VtxPosStagingBufferIndex], roResources.m_vtxPosBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_VtxNrmStagingBufferIndex], roResources.m_vtxNrmBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_VtxTangentsStagingBufferIndex], roResources.m_vtxTangentBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_VtxTexCoordStagingBufferIndex], roResources.m_vtxTexCoordBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_IndexStagingBufferIndex], roResources.m_idxBuffer.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_SurfaceStagingBufferIndex], roResources.m_surfaceBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_RenderStagingBufferIndex], roResources.m_renderBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_BoundingSphereBoundingIndex], roResources.m_boundingSpheres.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_LodStagingIndex], roResources.m_LODBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_MaterialStagingIndex], roResources.m_matBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		CreateResourcesTransitionBarrier(copyDestBarriers[CE_TERRAIN_VERTEX_SSBO_STAGING_IDX], roResources.m_terrainVtxBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		CreateResourcesTransitionBarrier(copyDestBarriers[CE_TERRAIN_VTX_IDX_SSBO_STAGING_IDX], roResources.m_terrainIdxBuffer.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		CreateResourcesTransitionBarrier(copyDestBarriers[CE_TERRAIN_HEIGHT_DATA_SSBO_STAGING_IDX], roResources.m_terrainHeightBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, 
			D3D12_RESOURCE_STATE_COPY_DEST);

		CreateResourcesTransitionBarrier(copyDestBarriers[CE_WORLD_VARIABLE_TRANSFORM_STAGING_IDX], cpuLogicBuffers.GPUSSBOWorldVariableTransform.buffer.Get(), D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST);

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			D3D12_RESOURCE_BARRIER clusterBufferBarrier{};
			CreateResourcesTransitionBarrier(clusterBufferBarrier, roResources.m_clusterVtxsBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
			copyDestBarriers.PushBack(clusterBufferBarrier);

			D3D12_RESOURCE_BARRIER clusterSpheresBarrier{};
			CreateResourcesTransitionBarrier(clusterSpheresBarrier, roResources.m_clusterSpheresBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
			copyDestBarriers.PushBack(clusterSpheresBarrier);

			D3D12_RESOURCE_BARRIER clusterConesBarrier{};
			CreateResourcesTransitionBarrier(clusterConesBarrier, roResources.m_clusterConesBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
			copyDestBarriers.PushBack(clusterConesBarrier);

			D3D12_RESOURCE_BARRIER clusterIdxBarrier{};
			CreateResourcesTransitionBarrier(clusterIdxBarrier, roResources.m_clusterIdxBuffer.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
			copyDestBarriers.PushBack(clusterIdxBarrier);
		}

		// execute
		roResources.BUFFER_COUNT = (UINT(copyDestBarriers.GetSize()));
		cmdContext.m_copyCmdList->ResourceBarrier(roResources.BUFFER_COUNT, copyDestBarriers.Data());

		// SRC BARRIERS
		BlitCL::DynamicArray<D3D12_RESOURCE_BARRIER> copySourceBarriers{ Ce_ConstDataSSBOCount, {} };

		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_VtxPosStagingBufferIndex], loadingContextMesh.m_vtxPosStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_VtxNrmStagingBufferIndex], loadingContextMesh.m_vtxNrmStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_VtxTangentsStagingBufferIndex], loadingContextMesh.m_vtxTngStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_VtxTexCoordStagingBufferIndex], loadingContextMesh.m_vtxTexCoordStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_IndexStagingBufferIndex], loadingContextMesh.m_vtxIdxStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_SurfaceStagingBufferIndex], surfaceStagingBuffer.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_RenderStagingBufferIndex], renderStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_BoundingSphereBoundingIndex], boundingSphereStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_LodStagingIndex], lodStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_MaterialStagingIndex], materialStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		CreateResourcesTransitionBarrier(copySourceBarriers[CE_TERRAIN_VERTEX_SSBO_STAGING_IDX], terrainVtxPosStagingBuffer.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		CreateResourcesTransitionBarrier(copySourceBarriers[CE_TERRAIN_VTX_IDX_SSBO_STAGING_IDX], terrainVtxIdxStagingBuffer.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		CreateResourcesTransitionBarrier(copySourceBarriers[CE_TERRAIN_HEIGHT_DATA_SSBO_STAGING_IDX], terrainHeightStaging.m_buffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		CreateResourcesTransitionBarrier(copySourceBarriers[CE_WORLD_VARIABLE_TRANSFORM_STAGING_IDX], worldVariableTransformStaging.m_buffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			D3D12_RESOURCE_BARRIER clusterStagingBarrier{};
			CreateResourcesTransitionBarrier(clusterStagingBarrier, clusterVtxStaging.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
			copySourceBarriers.PushBack(clusterStagingBarrier);

			D3D12_RESOURCE_BARRIER clusterSpheresStagingBarrier{};
			CreateResourcesTransitionBarrier(clusterSpheresStagingBarrier, clusterSpheresStaging.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
			copySourceBarriers.PushBack(clusterSpheresStagingBarrier);

			D3D12_RESOURCE_BARRIER clusterConesStagingBarrier{};
			CreateResourcesTransitionBarrier(clusterConesStagingBarrier, clusterConesStaging.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
			copySourceBarriers.PushBack(clusterConesStagingBarrier);

			D3D12_RESOURCE_BARRIER clusterIdxsStagingBarrier{};
			CreateResourcesTransitionBarrier(clusterIdxsStagingBarrier, clusterIdxStaging.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
			copySourceBarriers.PushBack(clusterIdxsStagingBarrier);
		}

		if (copySourceBarriers.GetSize() != roResources.BUFFER_COUNT)
		{
			BLIT_ERROR("SRC and DEST read only buffer count difference");
			return 0;
		}

		// execute
		cmdContext.m_copyCmdList->ResourceBarrier(Ce_ConstDataSSBOCount, copySourceBarriers.Data());

		cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_vtxPosBuffer.buffer.Get(), 0, loadingContextMesh.m_vtxPosStaging.m_buffer.Get(), 0, 
			sizeof(BlitzenEngine::VtxPos) * loadingContextMesh.m_vtxPosStaging.m_validDataIndex);
		cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_vtxNrmBuffer.buffer.Get(), 0, loadingContextMesh.m_vtxNrmStaging.m_buffer.Get(), 0, 
			sizeof(BlitzenEngine::VtxNormals) * loadingContextMesh.m_vtxNrmStaging.m_validDataIndex);
		cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_vtxTangentBuffer.buffer.Get(), 0, loadingContextMesh.m_vtxTngStaging.m_buffer.Get(), 0, 
			sizeof(BlitzenEngine::VtxTangents) * loadingContextMesh.m_vtxTngStaging.m_validDataIndex);
		cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_vtxTexCoordBuffer.buffer.Get(), 0, loadingContextMesh.m_vtxTexCoordStaging.m_buffer.Get(), 0, 
			sizeof(BlitzenEngine::VtxTexCoords) * loadingContextMesh.m_vtxTexCoordStaging.m_validDataIndex);
		cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_idxBuffer.m_buffer.Get(), 0, loadingContextMesh.m_vtxIdxStaging.m_buffer.Get(), 0, 
			sizeof(uint32_t) * loadingContextMesh.m_vtxIdxStaging.m_validDataIndex);
		cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_terrainVtxBuffer.buffer.Get(), 0, terrainVtxPosStagingBuffer.m_buffer.Get(), 0, 
			terrainVtxPosStagingBuffer.m_dataSize);
		cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_terrainIdxBuffer.m_buffer.Get(), 0, terrainVtxIdxStagingBuffer.m_buffer.Get(), 0,
			terrainVtxIdxStagingBuffer.m_dataSize);
		cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_terrainHeightBuffer.buffer.Get(), 0, terrainHeightStaging.m_buffer.Get(), 0,
			terrainHeightStaging.m_dataSize);
		cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_surfaceBuffer.buffer.Get(), 0, surfaceStagingBuffer.m_buffer.Get(), 0, surfaceStagingBuffer.m_dataSize);
		cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_renderBuffer.buffer.Get(), 0, 
			renderStaging.m_buffer.Get(), 0, renderStaging.m_dataSize);
		cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_boundingSpheres.buffer.Get(), 0, boundingSphereStaging.m_buffer.Get(), 0, boundingSphereStaging.m_dataSize);
		cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_LODBuffer.buffer.Get(), 0, lodStaging.m_buffer.Get(), 0, lodStaging.m_dataSize);
		cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_matBuffer.buffer.Get(), 0, materialStaging.m_buffer.Get(), 0, materialStaging.m_dataSize);
		cmdContext.m_copyCmdList->CopyBufferRegion(cpuLogicBuffers.GPUSSBOWorldVariableTransform.buffer.Get(), 0, worldVariableTransformStaging.m_buffer.Get(), 0, worldVariableTransformStaging.m_dataSize);
		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_clusterVtxsBuffer.buffer.Get(), 0, clusterVtxStaging.m_buffer.Get(), 0, clusterVtxStaging.m_dataSize);
			cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_clusterSpheresBuffer.buffer.Get(), 0, clusterSpheresStaging.m_buffer.Get(), 0, clusterSpheresStaging.m_dataSize);
			cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_clusterConesBuffer.buffer.Get(), 0, clusterConesStaging.m_buffer.Get(), 0, clusterConesStaging.m_dataSize);
			cmdContext.m_copyCmdList->CopyBufferRegion(roResources.m_clusterIdxBuffer.m_buffer.Get(), 0, clusterIdxStaging.m_buffer.Get(), 0, clusterIdxStaging.m_dataSize);
		}

		cmdContext.m_copyCmdList->Close();
		ID3D12CommandList* commandLists[] = { cmdContext.m_copyCmdList.Get() };
		commandQueue->ExecuteCommandLists(1, commandLists);

		PlaceFence(cmdContext.m_copyFence.m_value, commandQueue, cmdContext.m_copyFence.m_dx12Handle.Get(), cmdContext.m_copyFence.m_event);

		// success
		return 1;
	}

	void CreateResourceViews(ID3D12Device* device, DescriptorContext& ctx, CmdContext& cmdContext, ID3D12CommandQueue* queue, ReadOnlyResources& roResources, 
		ReadWriteResources* rwResourcesArray, CPU_LOGIC_BUFFERS& gameLogicBuffers, BlitzenEngine::DrawContext& context, DX12WRAPPER<ID3D12Resource>* pDepthTargets, UINT drawWidth, UINT drawHeight,
		LoadingContextMesh& loadingContextMesh)
	{
		BLIT_ASSERT(context.m_meshes.m_triangles.m_mapVtxCount == loadingContextMesh.m_vtxPosStaging.m_validDataIndex);
		BLIT_ASSERT(context.m_meshes.m_triangles.m_mapVtxCount == loadingContextMesh.m_vtxNrmStaging.m_validDataIndex);
		BLIT_ASSERT(context.m_meshes.m_triangles.m_mapVtxCount == loadingContextMesh.m_vtxTngStaging.m_validDataIndex);
		BLIT_ASSERT(context.m_meshes.m_triangles.m_mapVtxCount == loadingContextMesh.m_vtxTexCoordStaging.m_validDataIndex);
		BLIT_ASSERT(context.m_meshes.m_triangles.m_mapIdxCount == loadingContextMesh.m_vtxIdxStaging.m_validDataIndex);

		// DRAW DESCRIPTORS
		for (size_t i = 0; i < ce_framesInFlight; ++i)
		{
			ctx.m_vertexODSTableOffset[i] = ctx.m_viewHeapCurrentOffset;
			ctx.m_vertexODSTableHandle[i] = ctx.m_viewHeapHandle;
			ctx.m_vertexODSTableHandle[i].ptr += ctx.m_vertexODSTableOffset[i] * ctx.m_viewHeapIncrement;

			CreateBufferShaderResourceView(device, roResources.m_vtxPosBuffer.buffer.Get(), ctx, context.m_meshes.m_triangles.m_mapVtxCount, sizeof(BlitzenEngine::VtxPos));

			CreateBufferShaderResourceView(device, roResources.m_vtxNrmBuffer.buffer.Get(), ctx, context.m_meshes.m_triangles.m_mapVtxCount, sizeof(BlitzenEngine::VtxNormals));

			CreateBufferShaderResourceView(device, roResources.m_vtxTangentBuffer.buffer.Get(), ctx, context.m_meshes.m_triangles.m_mapVtxCount, sizeof(BlitzenEngine::VtxTangents));

			CreateBufferShaderResourceView(device, roResources.m_vtxTexCoordBuffer.buffer.Get(), ctx, context.m_meshes.m_triangles.m_mapVtxCount, sizeof(BlitzenEngine::VtxTexCoords));
		}

		// TERRAIN DESCRIPTORS
		ctx.m_terrainVertexTableOffset = ctx.m_viewHeapCurrentOffset;
		ctx.m_terrainVertexTableHandle = ctx.m_viewHeapHandle;
		ctx.m_terrainVertexTableHandle.ptr += ctx.m_terrainVertexTableOffset * ctx.m_viewHeapIncrement;

		CreateBufferShaderResourceView(device, roResources.m_terrainVtxBuffer.buffer.Get(), ctx, context.m_pTerrain->terrainVertexCount, sizeof(BlitzenEngine::VtxPos));

		// SHARED DESCRIPTORS
		for (size_t i = 0; i < ce_framesInFlight; ++i)
		{
			ctx.m_globalTableOffset[i] = ctx.m_viewHeapCurrentOffset;
			ctx.m_globalTableHandle[i] = ctx.m_viewHeapHandle;
			ctx.m_globalTableHandle[i].ptr += ctx.m_globalTableOffset[i] * ctx.m_viewHeapIncrement;

			auto& rwResources = rwResourcesArray[i];

			CreateBufferShaderResourceView(device, roResources.m_renderBuffer.buffer.Get(), ctx, BLIT_MAX_WORLD_RENDERS, sizeof(BlitzenEngine::RenderObject));

			CreateBufferUnorderedAccessView(device, ctx, rwResources.m_transformBuffer.buffer.Get(), nullptr, BLIT_MAX_WORLD_TRANSFORM_COUNT, sizeof(BlitzenEngine::MeshTransform), 0);

			CreateBufferShaderResourceView(device, roResources.m_surfaceBuffer.buffer.Get(), ctx, context.m_meshes.m_meshPrimitives.m_meshPrimitivesCount, sizeof(BlitzenEngine::PrimitiveSurface));

			CreateConstantBufferView(device, ctx, rwResources.m_viewBuffer.buffer.Get(), sizeof(BlitzenEngine::CameraViewData));
		}

		// CULLING GLOBAL DESCRIPTORS
		for (uint32_t frame = 0; frame < ce_framesInFlight; ++frame)
		{
			ctx.m_cullGlobalTableOffset[frame] = ctx.m_viewHeapCurrentOffset;
			ctx.m_cullGlobalTableHandle[frame] = ctx.m_viewHeapHandle;
			ctx.m_cullGlobalTableHandle[frame].ptr += ctx.m_cullGlobalTableOffset[frame] * ctx.m_viewHeapIncrement;

			auto& rwResources{ rwResourcesArray[frame] };

			CreateBufferShaderResourceView(device, roResources.m_LODBuffer.buffer.Get(), ctx, context.m_meshes.m_meshPrimitives.m_LODCount, sizeof(BlitzenEngine::LodData));

			CreateBufferShaderResourceView(device, roResources.m_boundingSpheres.buffer.Get(), ctx, BLIT_MAX_WORLD_RENDERS, sizeof(BlitzenEngine::BoundingSphere));
		}

		// CULLING DESCRIPTORS OPAQUE STATIC
		for (size_t i = 0; i < ce_framesInFlight; ++i)
		{
			ctx.m_cullOSTableOffset[i] = ctx.m_viewHeapCurrentOffset;
			ctx.m_cullOSTableHandle[i] = ctx.m_viewHeapHandle;
			ctx.m_cullOSTableHandle[i].ptr += ctx.m_cullOSTableOffset[i] * ctx.m_viewHeapIncrement;

			auto& rwResources = rwResourcesArray[i];

			CreateBufferUnorderedAccessView(device, ctx, rwResources.m_staticDrawCmdBuffer.buffer.Get(), rwResources.m_staticDrawCmdCounter.buffer.Get(), 
				BLIT_MAX_STATIC_DRAW_COMMANDS, sizeof(IndirectDrawCmd), 0);

			CreateBufferUnorderedAccessView(device, ctx, rwResources.m_staticDrawCmdCounter.buffer.Get(), nullptr, 1, sizeof(uint32_t), 0);
		}

		// CULLING DESCRIPTORS OPAQUE DYNAMIC
		for (uint32_t frame = 0; frame < ce_framesInFlight; ++frame)
		{
			ctx.m_cullODTableOffset[frame] = ctx.m_viewHeapCurrentOffset;
			ctx.m_cullODTableHandle[frame] = ctx.m_viewHeapHandle;
			ctx.m_cullODTableHandle[frame].ptr += ctx.m_cullODTableOffset[frame] * ctx.m_viewHeapIncrement;

			auto& rwResources{ rwResourcesArray[frame] };

			CreateBufferUnorderedAccessView(device, ctx, rwResources.m_dynamicDrawCmdBuffer.buffer.Get(), rwResources.m_dynamicDrawCmdCounter.buffer.Get(),
				BLIT_MAX_DYNAMIC_DRAW_COMMANDS, sizeof(IndirectDrawCmd), 0);

			CreateBufferUnorderedAccessView(device, ctx, rwResources.m_dynamicDrawCmdCounter.buffer.Get(), nullptr, 1, sizeof(uint32_t), 0);

			CreateBufferUnorderedAccessView(device, ctx, gameLogicBuffers.GPUSSBOWorldVariableTransform.buffer.Get(), nullptr, context.m_pResidents->m_transforms.m_moveableCount, 
				sizeof(BlitzenEngine::WVTransform), 0);

			CreateBufferShaderResourceView(device, roResources.m_terrainHeightBuffer.buffer.Get(), ctx, context.m_pTerrain->m_heightDataCount, sizeof(float));

			CreateBufferUnorderedAccessView(device, ctx, gameLogicBuffers.GPUSSBOWorldVariableMovement.buffer.Get(), nullptr, context.m_pResidents->m_transforms.m_moveableCount,
				sizeof(BlitzenEngine::WVMovement), 0);
		}

		// INSTANCING DESCRIPTORS
		if (BlitzenCore::Ce_InstanceCulling)
		{
			for (uint32_t frame = 0; frame < ce_framesInFlight; ++frame)
			{
				ctx.m_cullInstTableOffset[frame] = ctx.m_viewHeapCurrentOffset;
				ctx.m_cullInstTableHandle[frame] = ctx.m_viewHeapHandle;
				ctx.m_cullInstTableHandle[frame].ptr += ctx.m_cullInstTableOffset[frame] * ctx.m_viewHeapIncrement;

				auto& rwResources{ rwResourcesArray[frame] };

				CreateBufferUnorderedAccessView(device, ctx, rwResources.m_instanceDrawCmdBuffer.buffer.Get(), rwResources.m_instanceDrawCmdBuffer.buffer.Get(),
					BlitzenCore::Ce_MaxLodCountPerSurface, sizeof(IndirectDrawCmd), 0);

				CreateBufferUnorderedAccessView(device, ctx, roResources.m_instancedRenders.buffer.Get(), nullptr, 1, sizeof(uint32_t), 0);
			}
		}

		// VISIBILITY BUFFER FOR Double pass occlusion
		if constexpr (BlitzenCore::CE_OCCLUSION_DOUBLE_PASS)
		{
			for (uint32_t frame = 0; frame < ce_framesInFlight; ++frame)
			{
				auto& rwResources = rwResourcesArray[frame];

				ctx.m_cullOCCDPTableOffset[frame] = ctx.m_viewHeapCurrentOffset;
				ctx.m_cullOCCDPTableHandle[frame] = ctx.m_viewHeapHandle;
				ctx.m_cullOCCDPTableHandle[frame].ptr += ctx.m_cullOCCDPTableOffset[frame] * ctx.m_viewHeapIncrement;

				//CreateBufferUnorderedAccessView(device, ctx, rwResources.m_drawVisBuffer.buffer.Get(), nullptr, context.m_pResidents->m_renders.RENDER_COUNT, sizeof(uint32_t), 0);
			}
		}

		// CLUSTER DESCRIPTORS
		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			for (uint32_t i = 0; i < ce_framesInFlight; ++i)
			{
				auto& rwResources{ rwResourcesArray[i] };

				ctx.m_cullClusterTableOffset[i] = ctx.m_viewHeapCurrentOffset;
				ctx.m_cullClusterTableHandle[i] = ctx.m_viewHeapHandle;
				ctx.m_cullClusterTableHandle[i].ptr += ctx.m_cullClusterTableOffset[i] * ctx.m_viewHeapIncrement;

				CreateBufferUnorderedAccessView(device, ctx, rwResources.m_clusterDrawCmdBuffer.buffer.Get(), rwResources.m_clusterDrawCounter.buffer.Get(),
					BLIT_MAX_DYNAMIC_DRAW_COMMANDS, sizeof(IndirectDrawCmd), 0);

				CreateBufferUnorderedAccessView(device, ctx, rwResources.m_clusterDrawCounter.buffer.Get(), nullptr, 1, sizeof(uint32_t), 0);

				CreateBufferUnorderedAccessView(device, ctx, rwResources.m_clusterGroupDataBuffer.buffer.Get(), nullptr, Ce_ClusterGroupDataBufferSize, sizeof(ClusterGroupData), 0);

				CreateBufferUnorderedAccessView(device, ctx, rwResources.m_clusterGroupCounter.buffer.Get(), nullptr, 1, sizeof(ClusterDispatchCmd), 0);

				CreateBufferUnorderedAccessView(device, ctx, rwResources.m_clusterVisibilityBuffer.buffer.Get(), nullptr, Ce_ClusterGroupDataBufferSize * BLIT_MAX_CLUSTERS_PER_GROUP, 
					sizeof(BlitzenCore::FAT_BOOL), 0);

				CreateBufferShaderResourceView(device, roResources.m_clusterVtxsBuffer.buffer.Get(), ctx, context.m_meshes.m_clusters.m_clusterCount, sizeof(BlitzenEngine::ClusterVertices));

				CreateBufferShaderResourceView(device, roResources.m_clusterSpheresBuffer.buffer.Get(), ctx, context.m_meshes.m_clusters.m_clusterCount, sizeof(BlitzenEngine::ClusterSphere));

				CreateBufferShaderResourceView(device, roResources.m_clusterConesBuffer.buffer.Get(), ctx, context.m_meshes.m_clusters.m_clusterCount, sizeof(BlitzenEngine::ClusterCone));
			}
		}

		// HI_Z_MAP DESCRIPTORS
		if constexpr (BlitzenCore::Ce_Build_HI_Z)
		{
			CreateDepthPyramidDescriptors(device, rwResourcesArray, ctx, pDepthTargets, drawWidth, drawHeight);
		}
		
		// OPAQUE DRAW PS EXCLUSIVES
		ctx.m_pixelODSTableOffset = ctx.m_viewHeapCurrentOffset;
		ctx.m_pixelODSTableHandle = ctx.m_viewHeapHandle;
		ctx.m_pixelODSTableHandle.ptr += ctx.m_pixelODSTableOffset * ctx.m_viewHeapIncrement;

		CreateBufferShaderResourceView(device, roResources.m_matBuffer.buffer.Get(), ctx, context.m_textures.m_materialCount, sizeof(BlitzenEngine::Material));

		// TEXTURE DESCRIPTORS
		ctx.m_texturesTableOffset = ctx.m_viewHeapCurrentOffset;
		ctx.m_texturesTableHandle = ctx.m_viewHeapHandle;
		ctx.m_texturesTableHandle.ptr += ctx.m_texturesTableOffset * ctx.m_viewHeapIncrement;
		for (size_t i = 0; i < roResources.m_textureCount; ++i)
		{
			auto& tex2D{ roResources.m_drawTextures[i] };

			CreateTexture2DShaderResourceView(device, tex2D.resource.Get(), ctx, tex2D.format, tex2D.mipLevels);
		}
	}
}

#endif