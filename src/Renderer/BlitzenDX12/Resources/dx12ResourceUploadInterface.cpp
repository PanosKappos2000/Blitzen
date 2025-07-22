#if defined(_WIN32)
#include "dx12ResourcesUpload.h"
#include "Renderer/Interface/blitRenderer.h"
#include "Renderer/BlitzenDX12/Resources/dx12RNDResources.h"
#include "Renderer/BlitzenDX12/Resources/dx12Pipelines.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenEngine
{
	uint8_t UploadResourcesToGPU(BlitzenDX12::Dx12Renderer* pRenderer, DrawContext& drawContext, BlitzenDX12::LoadingContextMesh& loadingContextMesh, 
		BlitzenDX12::LoadingContextRenderObjects& loadingContextObj)
	{
		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			if (!BlitzenEngine::GenerateHLSLClusters(drawContext.m_meshes.m_clusters))
			{
				BLIT_ERROR("%s: Failed to generate HLSL clusters", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}
		}

		if (!BlitzenDX12::UploadResourcesToBuffers(pRenderer->m_device.Get(), drawContext, pRenderer->m_roResources, pRenderer->m_rwResources, pRenderer->MCpuLogicBuffers, pRenderer->m_cmdContext[0], 
			pRenderer->m_transferCommandQueue.Get(), loadingContextMesh, loadingContextObj))
		{
			BLIT_ERROR("%s: Failed to upload resources to GPU buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BlitzenDX12::CreateResourceViews(pRenderer->m_device.Get(), pRenderer->m_descriptorContext, pRenderer->m_cmdContext[pRenderer->m_currentFrame], pRenderer->m_transferCommandQueue.Get(), 
			pRenderer->m_roResources, pRenderer->m_rwResources, pRenderer->MCpuLogicBuffers, drawContext, pRenderer->m_depthBuffers, pRenderer->m_swapchainWidth, 
			pRenderer->m_swapchainHeight, loadingContextMesh);

		if (!BlitzenDX12::CheckForDeviceRemoval(pRenderer->m_device.Get()))
		{
			BLIT_ERROR("%s: Device removed, possibly after trying to create resources view", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		// Gives the pyramid size to th camera. There is not much reason for the camera to have it, but it is what it is.
		drawContext.m_camera.viewData.pyramidWidth = float(pRenderer->m_rwResources[0].m_HI_Z.width);
		drawContext.m_camera.viewData.pyramidHeight = float(pRenderer->m_rwResources[0].m_HI_Z.height);

		return 1;
	}

	uint8_t UploadTextureToGPU(BlitzenDX12::Dx12Renderer* pRenderer, void* pTextureData, const char* filepath)
	{
		BlitzenEngine::DDS_HEADER header{};
		BlitzenEngine::DDS_HEADER_DXT10 header10{};
		BlitzenPlatform::C_FILE_SCOPE scopedFILE{};
		BLIT_DXGI_FORMAT_COPY format{ BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_UNKNOWN };
		uint32_t blockSize;

		size_t imageSize = BlitzenEngine::LoadDDSImageData(header, header10, scopedFILE, format, pTextureData, blockSize, filepath);
		if(imageSize == BlitzenEngine::CE_LOAD_DDS_IMAGE_DATA_ERROR_CODE)
		{
			BLIT_ERROR("%s: Failed to load DDS image", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		return pRenderer->UploadTexture(pTextureData, header, header10, imageSize, blockSize, (DXGI_FORMAT)format);
	}

	uint8_t AllocateLoadingContextMesh(BlitzenDX12::Dx12Renderer* pRenderer, BlitzenDX12::LoadingContextMesh& ctx)
	{
		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_meshPrimStaging, BlitzenCore::Ce_MaxMeshPrimitivesCount, (PrimitiveSurface*)nullptr))
		{
			BLIT_FATAL("%s: Failed to create mesh primitives staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_lodDataStaging, BlitzenCore::Ce_MaxMeshPrimitivesCount * BlitzenCore::Ce_MaxLodCountPerSurface, (LodData*)nullptr))
		{
			BLIT_FATAL("%s: Failed to create Mesh Primitives Level of Detail data staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_vtxPosStaging, BlitzenEngine::Ce_MaxWorldVertexCount, (VtxPos*)nullptr))
		{
			BLIT_FATAL("%s: Failed to create Vertex Positions Staging Buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_vtxNrmStaging, BlitzenEngine::Ce_MaxWorldVertexCount, (VtxNormals*)nullptr))
		{
			BLIT_FATAL("%s: Failed to create Vertex Normals Staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_vtxTngStaging, BlitzenEngine::Ce_MaxWorldVertexCount, (VtxTangents*)nullptr))
		{
			BLIT_FATAL("%s: Failed to create Vertex Tangents Staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_vtxTexCoordStaging, BlitzenEngine::Ce_MaxWorldVertexCount, (VtxTexCoords*)nullptr))
		{
			BLIT_FATAL("%s: Failed to create Vertex Texture Coordinates staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_vtxIdxStaging, BlitzenEngine::Ce_MaxWorldVertexIndicesCount, (uint32_t*)nullptr))
		{
			BLIT_FATAL("%s: Failed to create Vertex Indices staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_clusterVtxStaging, CE_MAX_WORLD_CLUSTER_COUNT, (ClusterVertices*)nullptr))
			{
				BLIT_FATAL("%s: Failed to create Cluster Vertices staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_clusterSpheresStaging, CE_MAX_WORLD_CLUSTER_COUNT, (ClusterSphere*)nullptr))
			{
				BLIT_FATAL("%s: Failed to create Cluster Spheres staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_clusterConesStaging, CE_MAX_WORLD_CLUSTER_COUNT, (ClusterCone*)nullptr))
			{
				BLIT_FATAL("%s: Failed to create Cluster Cones staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_clusterIdxStaging, BlitzenEngine::Ce_MaxWorldVertexIndicesCount, (uint32_t*)nullptr))
			{
				BLIT_FATAL("%s: Failed to create Cluster Indices staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}
		}

		// Success
		return 1;
	}

	/***********************************************************************************************************
	*	THE BELOW FUNCTIONS ARE USED TO UPLOAD DATA TO STAGING BUFFERS.										   *
	*	While they are very repitive and I could make them more generic (eg. use templates or void), 		   *
	*	Tha would require access to Dx12 on engine code directly, which I would rather avoid.				   *
	************************************************************************************************************/
	uint8_t UploadToMeshPrimitiveStagingBuffer(BlitzenDX12::LoadingContextMesh& ctx, PrimitiveSurface* primitives, uint32_t count)
	{
		SIZE_T copySize{ sizeof(PrimitiveSurface) * count };
		if ((sizeof(PrimitiveSurface) * ctx.m_meshPrimStaging.m_validDataIndex) + copySize > ctx.m_meshPrimStaging.m_dataSize)
		{
			BLIT_FATAL("%s: Mesh Primitive Staging buffer data overflow", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BLIT_ASSERT_MESSAGE(primitives != nullptr, "Invalid array handle passed for primitive surfaces (STAGING BUFFER UPLOAD)");

		auto pDest{ &ctx.m_meshPrimStaging.m_pMapped[ctx.m_meshPrimStaging.m_validDataIndex] };
		BlitzenCore::MANUAL_COPY(pDest, primitives, copySize);

		// success
		ctx.m_meshPrimStaging.m_validDataIndex += count;
		return 1;
	}

	uint8_t UploadToLODDataStagingBuffer(BlitzenDX12::LoadingContextMesh& ctx, LodData* LODs, uint32_t count)
	{
		SIZE_T copySize{ sizeof(LodData) * count };
		if ((sizeof(LodData) * ctx.m_lodDataStaging.m_validDataIndex) + copySize > ctx.m_lodDataStaging.m_dataSize)
		{
			BLIT_FATAL("%s: Lod Data Staging buffer overflow", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BLIT_ASSERT_MESSAGE(LODs != nullptr, "Invalid array handle passed for Mesh Primitives Levels of Details (STAGING BUFFER UPLOAD)");

		auto pDest{ &ctx.m_lodDataStaging.m_pMapped[ctx.m_lodDataStaging.m_validDataIndex] };
		BlitzenCore::MANUAL_COPY(pDest, LODs, copySize);

		// success
		ctx.m_lodDataStaging.m_validDataIndex += count;
		return 1;
	}

	uint8_t UploadToVertexPositionsStagingBuffer(BlitzenDX12::LoadingContextMesh& ctx, VtxPos* vtxPositions, uint32_t count)
	{
		SIZE_T copySize{ sizeof(VtxPos) * count };
		if ((sizeof(VtxPos) * ctx.m_vtxPosStaging.m_validDataIndex) + copySize > ctx.m_vtxPosStaging.m_dataSize)
		{
			BLIT_FATAL("%s: Vertex positions staging buffer overflow", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BLIT_ASSERT_MESSAGE(vtxPositions != nullptr, "Invalid array handle passed for Vertex Positions (STAGING BUFFER UPLOAD)");

		auto pDest{ &ctx.m_vtxPosStaging.m_pMapped[ctx.m_vtxPosStaging.m_validDataIndex] };
		BlitzenCore::MANUAL_COPY(pDest, vtxPositions, copySize);

		// success
		ctx.m_vtxPosStaging.m_validDataIndex += count;
		return 1;
	}

	uint8_t UploadToVertexNormalsStagingBuffer(BlitzenDX12::LoadingContextMesh& ctx, VtxNormals* vtxNormals, uint32_t count)
	{
		SIZE_T copySize{ sizeof(VtxNormals) * count };
		if ((sizeof(VtxNormals) * ctx.m_vtxNrmStaging.m_validDataIndex) + copySize > ctx.m_vtxNrmStaging.m_dataSize)
		{
			BLIT_FATAL("%s: Vertex Normals staging buffer overflow", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BLIT_ASSERT_MESSAGE(vtxNormals != nullptr, "Invalid array handle passed for Vertex Normals (STAGING BUFFER UPLOAD)");

		auto pDest{ &ctx.m_vtxNrmStaging.m_pMapped[ctx.m_vtxNrmStaging.m_validDataIndex] };
		BlitzenCore::MANUAL_COPY(pDest, vtxNormals, copySize);

		// success
		ctx.m_vtxNrmStaging.m_validDataIndex += count;
		return 1;
	}

	uint8_t UploadToVertexTangentsStagingBuffer(BlitzenDX12::LoadingContextMesh& ctx, VtxTangents* vtxTangents, uint32_t count)
	{
		SIZE_T copySize{ sizeof(VtxTangents) * count };
		if ((sizeof(VtxTangents) * ctx.m_vtxTngStaging.m_validDataIndex) + copySize > ctx.m_vtxTngStaging.m_dataSize)
		{
			BLIT_FATAL("%s: Vertex Tangents staging buffer overflow", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BLIT_ASSERT_MESSAGE(vtxTangents != nullptr, "Ivalid array handle for Vertex Tangents (STAGING BUFFER UPLOAD)");

		auto pDest{ &ctx.m_vtxTngStaging.m_pMapped[ctx.m_vtxTngStaging.m_validDataIndex] };
		BlitzenCore::MANUAL_COPY(pDest, vtxTangents, copySize);

		// success 
		ctx.m_vtxTngStaging.m_validDataIndex += count;
		return 1;
	}

	uint8_t UploadToVertexTextureCoordinatesStagingBuffer(BlitzenDX12::LoadingContextMesh& ctx, VtxTexCoords* vtxTexCoords, uint32_t count)
	{
		SIZE_T copySize{ sizeof(VtxTexCoords) * count };
		if ((sizeof(VtxTexCoords) * ctx.m_vtxTexCoordStaging.m_validDataIndex) + copySize > ctx.m_vtxTexCoordStaging.m_dataSize)
		{
			BLIT_FATAL("%s: Vertex Texture coordinates staging buffer overflow", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BLIT_ASSERT_MESSAGE(vtxTexCoords != nullptr, "Invalid array handle for Vertex Texture Coordinates (STAGING BUFFER UPLOAD)");

		auto pDest{ &ctx.m_vtxTexCoordStaging.m_pMapped[ctx.m_vtxTexCoordStaging.m_validDataIndex] };
		BlitzenCore::MANUAL_COPY(pDest, vtxTexCoords, copySize);

		// success
		ctx.m_vtxTexCoordStaging.m_validDataIndex += count;
		return 1;
	}

	uint8_t UploadToVertexIndicesStagingBuffer(BlitzenDX12::LoadingContextMesh& ctx, uint32_t* indices, uint32_t count)
	{
		SIZE_T copySize{ sizeof(uint32_t) * count };
		if ((sizeof(uint32_t) * ctx.m_vtxIdxStaging.m_validDataIndex) + copySize > ctx.m_vtxIdxStaging.m_dataSize)
		{
			BLIT_FATAL("%s: Vertex Idices staging buffer overflow", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BLIT_ASSERT_MESSAGE(indices != nullptr, "Invalid array handle for Vertex Indices (STAGING BUFFER UPLOAD)");

		auto pDest{ &ctx.m_vtxIdxStaging.m_pMapped[ctx.m_vtxIdxStaging.m_validDataIndex] };
		BlitzenCore::MANUAL_COPY(pDest, indices, copySize);

		// success
		ctx.m_vtxIdxStaging.m_validDataIndex += count;
		return 1;
	}

	uint8_t UploadToClusterVerticesStagingBuffer(BlitzenDX12::LoadingContextMesh& ctx, ClusterVertices* clusterVertices, uint32_t count)
	{
		BLIT_ASSERT_MESSAGE(BlitzenCore::Ce_BuildClusters, "Clusters not requested, but renderer tried to upload Cluster Vertices to staging buffer");

		SIZE_T copySize{ sizeof(ClusterVertices) * count };
		if ((sizeof(ClusterVertices) * ctx.m_clusterVtxStaging.m_validDataIndex) + copySize > ctx.m_clusterVtxStaging.m_dataSize)
		{
			BLIT_FATAL("%s: Cluster Vertices staging buffer overflow", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BLIT_ASSERT_MESSAGE(clusterVertices != nullptr, "Invalid array handle for cluster vertices (STAGING BUFFER UPLOAD)");

		auto pDest{ &ctx.m_clusterVtxStaging.m_pMapped[ctx.m_clusterVtxStaging.m_validDataIndex] };
		BlitzenCore::MANUAL_COPY(pDest, clusterVertices, copySize);

		// success
		ctx.m_clusterVtxStaging.m_validDataIndex += count;
		return 1;
	}

	uint8_t UploadToClusterSpheresStagingBuffer(BlitzenDX12::LoadingContextMesh& ctx, ClusterSphere* clusterSpheres, uint32_t count)
	{
		BLIT_ASSERT_MESSAGE(BlitzenCore::Ce_BuildClusters, "Clusters not requested, but renderer tried to upload Cluster Spheres to staging buffer");

		SIZE_T copySize{ sizeof(ClusterSphere) * count };
		if ((sizeof(ClusterSphere) * ctx.m_clusterSpheresStaging.m_validDataIndex) + copySize > ctx.m_clusterSpheresStaging.m_dataSize)
		{
			BLIT_FATAL("%s: Cluster Spheres staging buffer overflow", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BLIT_ASSERT_MESSAGE(clusterSpheres != nullptr, "Invalid array handle for cluster spheres (STAGING BUFFER UPLOAD)");

		auto pDest{ &ctx.m_clusterSpheresStaging.m_pMapped[ctx.m_clusterSpheresStaging.m_validDataIndex] };
		BlitzenCore::MANUAL_COPY(pDest, clusterSpheres, copySize);

		// success
		ctx.m_clusterSpheresStaging.m_validDataIndex += count;
		return 1;
	}

	uint8_t UploadToClusterConesStagingBuffer(BlitzenDX12::LoadingContextMesh& ctx, ClusterCone* clusterCones, uint32_t count)
	{
		BLIT_ASSERT_MESSAGE(BlitzenCore::Ce_BuildClusters, "Clusters not requested, but renderer tried to upload Cluster Cones to staging buffer");

		SIZE_T copySize{ sizeof(ClusterCone) * count };
		if ((sizeof(ClusterCone) * ctx.m_clusterConesStaging.m_validDataIndex) + copySize > ctx.m_clusterConesStaging.m_dataSize)
		{
			BLIT_FATAL("%s: Cluster Cones staging buffer overflow", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BLIT_ASSERT_MESSAGE(clusterCones != nullptr, "Invalid array handle for cluster cones (STAGING BUFFER UPLOAD)");

		auto pDest{ &ctx.m_clusterConesStaging.m_pMapped[ctx.m_clusterConesStaging.m_validDataIndex] };
		BlitzenCore::MANUAL_COPY(pDest, clusterCones, copySize);

		// success
		ctx.m_clusterConesStaging.m_validDataIndex += count;
		return 1;
	}

	uint8_t UploadToClusterIndicesStagingBuffer(BlitzenDX12::LoadingContextMesh& ctx, uint32_t* clusterIndices, uint32_t count)
	{
		BLIT_ASSERT_MESSAGE(BlitzenCore::Ce_BuildClusters, "Clusters not requested, but renderer tried to upload Cluster Indices to staging buffer");

		SIZE_T copySize{ sizeof(uint32_t) * count };
		if ((sizeof(uint32_t) * ctx.m_clusterIdxStaging.m_validDataIndex) + copySize > ctx.m_clusterIdxStaging.m_dataSize)
		{
			BLIT_FATAL("%s: Cluster Indices staging buffer overflow", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BLIT_ASSERT_MESSAGE(clusterIndices != nullptr, "Invalid array handle for cluster indices (STAGING BUFFER UPLOAD)");

		auto pDest{ &ctx.m_clusterIdxStaging.m_pMapped[ctx.m_clusterIdxStaging.m_validDataIndex] };
		BlitzenCore::MANUAL_COPY(pDest, clusterIndices, copySize);

		ctx.m_clusterIdxStaging.m_validDataIndex += count;
		return 1;
	}


	// ... should have been templated
	static uint8_t UploadMeshPrimitiveResourcesToStagingBuffer(BLIT_STRAIGHTHANDLE pDest, BLIT_STRAIGHTHANDLE pSrc, uint32_t offset, uint32_t count, SIZE_T dataSize, SIZE_T limit)
	{
		SIZE_T copySize{ dataSize * count };
		if ((dataSize * offset) + copySize > limit)
		{
			BLIT_FATAL("%s: Staging buffer overflow", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BLIT_ASSERT_MESSAGE(pSrc != nullptr, "Invalid array handle (STAGING BUFFER UPLOAD)");

		auto pDestWithOffset{ &reinterpret_cast<uint8_t*>(pDest)[offset] };
		BlitzenCore::MANUAL_COPY(pDestWithOffset, pSrc, copySize);

		return 1;
	}

	uint8_t UploadMeshPrimitiveResourcesToStagingBufferGeneral(BlitzenDX12::LoadingContextMesh& ctx, BLIT_STRAIGHTHANDLE pData, uint32_t count, MESH_RESOURCES_STAGING_BUFFER_RESOURCE_TYPE resourceType)
	{
		switch (resourceType)
		{
		case MESH_RESOURCES_STAGING_BUFFER_RESOURCE_TYPE::VERTEX_POSITIONS:
		{
			if (!UploadMeshPrimitiveResourcesToStagingBuffer(ctx.m_vtxPosStaging.m_pMapped, pData, ctx.m_vtxPosStaging.m_validDataIndex, count, sizeof(VtxPos), ctx.m_vtxPosStaging.m_dataSize))
			{
				BLIT_FATAL("%s: Mesh Primitive Staging buffer Upload Failure", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			// success
			ctx.m_vtxPosStaging.m_validDataIndex += count;
			return 1;
		}
		default:
		{
			BLIT_FATAL("%s: USE EXPLICIT API FOR STAGING BUFFER UPLOAD", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}
		}
	}

	uint8_t AllocateLoadingContextRenderObjects(BlitzenDX12::Dx12Renderer* pRenderer, BlitzenDX12::LoadingContextRenderObjects& ctx)
	{
		if(!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_renderStaging, BLIT_MAX_WORLD_OPAQUE_STATIC_RENDERS, (RenderObject*)nullptr))
		{
			BLIT_FATAL("%s: Failed to allocate render object staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_dynamicRenderStaging, BLIT_MAX_WORLD_OPAQUE_DYNAMIC_RENDERS, (RenderObject*)nullptr))
		{
			BLIT_FATAL("%s: Failed to allocate dynamic render object staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_transformStaging, BLIT_MAX_WORLD_TRANSFORM_COUNT, (MeshTransform*)nullptr))
		{
			BLIT_FATAL("%s: Failed to allocate world transform staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}
		
		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_cpuTransformStaging, BLIT_MAX_WORLD_TRANSFORM_COUNT, (WVTransform*)nullptr))
		{
			BLIT_FATAL("%s: Failed to allocate CPU_DATA transform staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.mColliderAMaxRadStaging, CE_MAX_WORLD_COLLIDER_COUNT, (ColliderAMaxRad*)nullptr))
		{
			BLIT_FATAL("%s: Failed to allocate 16 byte Collider data 1st part staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.mColliderBMinTypeStaging, CE_MAX_WORLD_COLLIDER_COUNT, (ColliderBMinType*)nullptr))
		{
			BLIT_FATAL("%s: Failed to allocate 16 byte Collider data 2nd part staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}
		
		return 1;
	}

	uint8_t UploadToRenderObjectStagingBuffer(BlitzenDX12::LoadingContextRenderObjects& ctx, RenderObject* renderObjects, uint32_t renderCount)
	{
		BLIT_ASSERT_MESSAGE(false, "MISSING IMPLEMENTATION");
		return 1;
	}

	uint8_t UploadToDynamicRenderObjectStagingBuffer(BlitzenDX12::LoadingContextRenderObjects& ctx, RenderObject* renderObjects, uint32_t renderCount)
	{
		BLIT_ASSERT_MESSAGE(false, "MISSING IMPLEMENTATION");
		return 1;
	}

	uint8_t UploadToWorldTransformStagingBuffer(BlitzenDX12::LoadingContextRenderObjects& ctx, MeshTransform* transforms, uint32_t transformCount)
	{
		BLIT_ASSERT_MESSAGE(false, "MISSING IMPLEMENTATION");
		return 1;
	}

	uint8_t UploadToCPUTransformStagingBuffer(BlitzenDX12::LoadingContextRenderObjects& ctx, WVTransform* transforms, uint32_t transformCount)
	{
		BLIT_ASSERT_MESSAGE(false, "MISSING IMPLEMENTATION");
		return 1;
	}

	uint8_t UploadToRenderObjectStagingBuffer_MKII(BlitzenDX12::Dx12Renderer* pRenderer, BlitzenDX12::LoadingContextRenderObjects& ctx, RenderObject* renderObjects, uint32_t renderCount)
	{
		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_renderStaging, BLIT_MAX_WORLD_OPAQUE_STATIC_RENDERS, (RenderObject*)nullptr))
		{
			BLIT_FATAL("%s: Failed to allocate render object staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		SIZE_T copySize{ sizeof(BlitzenEngine::RenderObject) * renderCount };
		if ((sizeof(BlitzenEngine::RenderObject) * ctx.m_renderStaging.m_validDataIndex) + copySize > ctx.m_renderStaging.m_dataSize)
		{
			BLIT_FATAL("%s: Render Object staging buffer overflow", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BLIT_ASSERT_MESSAGE(renderObjects != nullptr, "Invalid array handle for render objects (STAGING BUFFER UPLOAD)");

		auto pDest{ &ctx.m_renderStaging.m_pMapped[ctx.m_renderStaging.m_validDataIndex] };
		BlitzenCore::MANUAL_COPY(pDest, renderObjects, copySize);

		ctx.m_renderStaging.m_validDataIndex += renderCount;

		return 1;
	}

	uint8_t UploadToDynamicRenderObjectStagingBuffer_MKII(BlitzenDX12::Dx12Renderer* pRenderer, BlitzenDX12::LoadingContextRenderObjects& ctx, RenderObject* renderObjects, uint32_t renderCount)
	{
		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_dynamicRenderStaging, BLIT_MAX_WORLD_OPAQUE_DYNAMIC_RENDERS, (RenderObject*)nullptr))
		{
			BLIT_FATAL("%s: Failed to allocate dynamic render object staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		SIZE_T copySize{ sizeof(BlitzenEngine::RenderObject) * renderCount };
		if ((sizeof(BlitzenEngine::RenderObject) * ctx.m_dynamicRenderStaging.m_validDataIndex) + copySize > ctx.m_dynamicRenderStaging.m_dataSize)
		{
			BLIT_FATAL("%s: Dynamic Render Object staging buffer overflow", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BLIT_ASSERT_MESSAGE(renderObjects != nullptr, "Invalid array handle for render objects (STAGING BUFFER UPLOAD)");

		auto pDest{ &ctx.m_dynamicRenderStaging.m_pMapped[ctx.m_dynamicRenderStaging.m_validDataIndex] };
		BlitzenCore::MANUAL_COPY(pDest, renderObjects, copySize);

		ctx.m_dynamicRenderStaging.m_validDataIndex += renderCount;

		return 1;
	}

	uint8_t UploadToWorldTransformStagingBuffer_MKII(BlitzenDX12::Dx12Renderer* pRenderer, BlitzenDX12::LoadingContextRenderObjects& ctx, MeshTransform* transforms, uint32_t transformCount)
	{
		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_transformStaging, BLIT_MAX_WORLD_TRANSFORM_COUNT, (MeshTransform*)nullptr))
		{
			BLIT_FATAL("%s: Failed to allocate world transform staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		SIZE_T copySize{ sizeof(BlitzenEngine::MeshTransform) * transformCount };
		if ((sizeof(BlitzenEngine::MeshTransform) * ctx.m_transformStaging.m_validDataIndex) + copySize > ctx.m_transformStaging.m_dataSize)
		{
			BLIT_FATAL("%s: World Transform staging buffer overflow", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BLIT_ASSERT_MESSAGE(transforms != nullptr, "Invalid array handle for world transforms (STAGING BUFFER UPLOAD)");

		auto pDest{ &ctx.m_transformStaging.m_pMapped[ctx.m_transformStaging.m_validDataIndex] };
		BlitzenCore::MANUAL_COPY(pDest, transforms, copySize);

		ctx.m_transformStaging.m_validDataIndex += transformCount;

		return 1;
	}

	uint8_t UploadToCPUTransformStagingBuffer_MKII(BlitzenDX12::Dx12Renderer* pRenderer, BlitzenDX12::LoadingContextRenderObjects& ctx, WVTransform* transforms, uint32_t transformCount)
	{
		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_cpuTransformStaging, BLIT_MAX_WORLD_VARIABLE_COUNT, (WVTransform*)nullptr))
		{
			BLIT_FATAL("%s: Failed to allocate CPU_DATA transform staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		SIZE_T copySize{ sizeof(WVTransform) * transformCount };
		if((sizeof(WVTransform) * ctx.m_cpuTransformStaging.m_validDataIndex) + copySize > ctx.m_cpuTransformStaging.m_dataSize)
		{
			BLIT_FATAL("%s: CPU_DATA Transform staging buffer overflow", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BLIT_ASSERT_MESSAGE(transforms != nullptr, "Invalid array handle for CPU_DATA transforms (STAGING BUFFER UPLOAD)");

		auto pDest{ &ctx.m_cpuTransformStaging.m_pMapped[ctx.m_cpuTransformStaging.m_validDataIndex] };
		BlitzenCore::MANUAL_COPY(pDest, transforms, copySize);

		ctx.m_cpuTransformStaging.m_validDataIndex += transformCount;

		return 1;
	}

	uint8_t UploadToBoundingSpheresStagingBuffer_MKII(BlitzenDX12::Dx12Renderer* pRenderer, BlitzenDX12::LoadingContextRenderObjects& ctx, BoundingSphere* spheres, uint32_t count)
	{
		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_boundingSpheresStaging, BLIT_MAX_WORLD_TRANSFORM_COUNT, (BoundingSphere*)nullptr))
		{
			BLIT_FATAL("%s: Failed to allocate Bounding sphere staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		SIZE_T copySize{ sizeof(BoundingSphere) * count };
		if ((sizeof(BoundingSphere) * ctx.m_boundingSpheresStaging.m_validDataIndex) + copySize > ctx.m_boundingSpheresStaging.m_dataSize)
		{
			BLIT_FATAL("%s: Bounding Spheres staging buffer overflow", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BLIT_ASSERT_MESSAGE(spheres != nullptr, "Invalid array handle for bounding spheres (STAGING BUFFER UPLOAD)");

		auto pDest{ &ctx.m_boundingSpheresStaging.m_pMapped[ctx.m_boundingSpheresStaging.m_validDataIndex] };
		BlitzenCore::MANUAL_COPY(pDest, spheres, copySize);

		ctx.m_boundingSpheresStaging.m_validDataIndex += count;

		return 1;
	}

	uint8_t UploadToColliderAMaxRadStagingBuffer_MKII(BlitzenDX12::Dx12Renderer* pRenderer, RenderingLoadingContextRenderObjects& ctx, ColliderAMaxRad* data, uint32_t count)
	{
		using InnerFunctionType = ColliderAMaxRad;

		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.mColliderAMaxRadStaging, CE_MAX_WORLD_COLLIDER_COUNT, (InnerFunctionType*)nullptr))
		{
			BLIT_FATAL("%s: Failed to allocate Collider data (1st 16bytes | AMaxRad) staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		SIZE_T copySize{ sizeof(InnerFunctionType) * count};
		if (sizeof(InnerFunctionType) * ctx.mColliderAMaxRadStaging.m_validDataIndex + copySize > ctx.mColliderAMaxRadStaging.m_dataSize)
		{
			BLIT_FATAL("%s: Collider data AMaxRad staging buffer overflow", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BLIT_ASSERT_MESSAGE(data != nullptr, "Invalid array handle for Collider Data AMaxRad (STAGING BUFFER UPLOAD)");

		auto pDest{ &ctx.mColliderAMaxRadStaging.m_pMapped[ctx.mColliderAMaxRadStaging.m_validDataIndex] };
		BlitzenCore::MANUAL_COPY(pDest, data, copySize);

		ctx.mColliderAMaxRadStaging.m_validDataIndex += count;

		return 1;
	}

	uint8_t UploadToColliderBMinTypeStagingBuffer_MKII(BlitzenDX12::Dx12Renderer* pRenderer, RenderingLoadingContextRenderObjects& ctx, ColliderBMinType* data, uint32_t count)
	{
		using InnerFunctionType = ColliderBMinType;

		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.mColliderBMinTypeStaging, CE_MAX_WORLD_COLLIDER_COUNT, (InnerFunctionType*)nullptr))
		{
			BLIT_FATAL("%s: Failed to allocate Collider data (2nd 16bytes | BMinType) staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		SIZE_T copySize{ sizeof(InnerFunctionType) * count };
		if (sizeof(InnerFunctionType) * ctx.mColliderBMinTypeStaging.m_validDataIndex + copySize > ctx.mColliderBMinTypeStaging.m_dataSize)
		{
			BLIT_FATAL("%s: Collider data BMinType staging buffer overflow", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BLIT_ASSERT_MESSAGE(data != nullptr, "Invalid array handle for Collider Data BMinType (STAGING BUFFER UPLOAD)");

		auto pDest{ &ctx.mColliderBMinTypeStaging.m_pMapped[ctx.mColliderBMinTypeStaging.m_validDataIndex] };
		BlitzenCore::MANUAL_COPY(pDest, data, copySize);

		ctx.mColliderBMinTypeStaging.m_validDataIndex += count;

		return 1;
		return 1;
	}

	uint8_t UploadNewGeometryDataToSSBOs(BlitzenDX12::Dx12Renderer* pRenderer, BlitzenDX12::LoadingContextRenderObjects& instanceData, BlitzenDX12::LoadingContextMesh& resourceData)
	{
		constexpr uint32_t CE_STAGING_BUFFER_BARRIER_COUNT = 50;
		constexpr uint32_t CE_SSBO_BARRIER_ARRAY_OFFSET = CE_STAGING_BUFFER_BARRIER_COUNT;
		constexpr uint32_t CE_SSBO_BARRIER_COUNT = CE_STAGING_BUFFER_BARRIER_COUNT * BlitzenDX12::ce_framesInFlight;
		uint32_t validBarrierCount = 0;
		D3D12_RESOURCE_BARRIER stagingResourceBarriers[CE_SSBO_BARRIER_COUNT + CE_STAGING_BUFFER_BARRIER_COUNT]{};

		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], resourceData.m_vtxPosStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], resourceData.m_vtxNrmStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], resourceData.m_vtxTngStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], resourceData.m_vtxTexCoordStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], resourceData.m_vtxIdxStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], resourceData.m_clusterVtxStaging.m_buffer.Get(), 
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
			BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], resourceData.m_clusterSpheresStaging.m_buffer.Get(), 
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
			BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], resourceData.m_clusterConesStaging.m_buffer.Get(), 
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
			BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], resourceData.m_clusterIdxStaging.m_buffer.Get(), 
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		}
		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], resourceData.m_meshPrimStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], resourceData.m_lodDataStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], instanceData.m_renderStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], instanceData.m_dynamicRenderStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], instanceData.m_transformStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], instanceData.m_cpuTransformStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], instanceData.m_boundingSpheresStaging.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], pRenderer->m_roResources.m_vtxPosBuffer.buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], pRenderer->m_roResources.m_vtxNrmBuffer.buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], pRenderer->m_roResources.m_vtxTangentBuffer.buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], pRenderer->m_roResources.m_vtxTexCoordBuffer.buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], pRenderer->m_roResources.m_idxBuffer.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], pRenderer->m_roResources.m_clusterVtxsBuffer.buffer.Get(), 
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
			BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], pRenderer->m_roResources.m_clusterSpheresBuffer.buffer.Get(), 
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
			BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], pRenderer->m_roResources.m_clusterConesBuffer.buffer.Get(), 
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
			BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], pRenderer->m_roResources.m_clusterIdxBuffer.m_buffer.Get(), 
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		}
		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], pRenderer->m_roResources.m_surfaceBuffer.buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], pRenderer->m_roResources.m_LODBuffer.buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], pRenderer->m_roResources.m_renderBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], pRenderer->MCpuLogicBuffers.GPUSSBOWorldVariableTransform.buffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		for (uint32_t frame = 0; frame < BlitzenDX12::ce_framesInFlight; frame++)
		{
			BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], pRenderer->m_rwResources[frame].m_transformBuffer.buffer.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		}
		BlitzenDX12::CreateResourcesTransitionBarrier(stagingResourceBarriers[validBarrierCount++], pRenderer->m_roResources.m_boundingSpheres.buffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		auto cmdList = pRenderer->m_cmdContext[0].m_copyCmdList.Get();

		pRenderer->m_cmdContext[0].m_copyCmdAlloc.Reset();
		cmdList->Reset(pRenderer->m_cmdContext[0].m_copyCmdAlloc.Get(), nullptr);

		cmdList->ResourceBarrier(validBarrierCount, stagingResourceBarriers);

		cmdList->CopyBufferRegion(pRenderer->m_roResources.m_vtxPosBuffer.buffer.Get(), 0, resourceData.m_vtxIdxStaging.m_buffer.Get(), 0, 
			resourceData.m_vtxIdxStaging.m_validDataIndex * sizeof(VtxPos));
		cmdList->CopyBufferRegion(pRenderer->m_roResources.m_vtxNrmBuffer.buffer.Get(), 0, resourceData.m_vtxNrmStaging.m_buffer.Get(), 0, 
			resourceData.m_vtxNrmStaging.m_validDataIndex * sizeof(VtxNormals));
		cmdList->CopyBufferRegion(pRenderer->m_roResources.m_vtxTangentBuffer.buffer.Get(), 0, resourceData.m_vtxTngStaging.m_buffer.Get(), 0, 
			resourceData.m_vtxTngStaging.m_validDataIndex * sizeof(VtxTangents));
		cmdList->CopyBufferRegion(pRenderer->m_roResources.m_vtxTexCoordBuffer.buffer.Get(), 0, resourceData.m_vtxTexCoordStaging.m_buffer.Get(), 0, 
			resourceData.m_vtxTexCoordStaging.m_validDataIndex * sizeof(VtxTexCoords));
		cmdList->CopyBufferRegion(pRenderer->m_roResources.m_idxBuffer.m_buffer.Get(), 0, resourceData.m_vtxIdxStaging.m_buffer.Get(), 0, 
			resourceData.m_vtxIdxStaging.m_validDataIndex * sizeof(uint32_t));
		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			cmdList->CopyBufferRegion(pRenderer->m_roResources.m_clusterVtxsBuffer.buffer.Get(), 0, resourceData.m_clusterVtxStaging.m_buffer.Get(), 0, 
				resourceData.m_clusterVtxStaging.m_validDataIndex * sizeof(ClusterVertices));
			cmdList->CopyBufferRegion(pRenderer->m_roResources.m_clusterSpheresBuffer.buffer.Get(), 0, resourceData.m_clusterSpheresStaging.m_buffer.Get(), 0,
				resourceData.m_clusterSpheresStaging.m_validDataIndex * sizeof(ClusterSphere));
			cmdList->CopyBufferRegion(pRenderer->m_roResources.m_clusterConesBuffer.buffer.Get(), 0, resourceData.m_clusterConesStaging.m_buffer.Get(), 0, 
				resourceData.m_clusterConesStaging.m_validDataIndex * sizeof(ClusterCone));
			cmdList->CopyBufferRegion(pRenderer->m_roResources.m_clusterIdxBuffer.m_buffer.Get(), 0, resourceData.m_clusterIdxStaging.m_buffer.Get(), 0,
				resourceData.m_clusterIdxStaging.m_validDataIndex * sizeof(uint32_t));
		}
		cmdList->CopyBufferRegion(pRenderer->m_roResources.m_surfaceBuffer.buffer.Get(), 0, resourceData.m_meshPrimStaging.m_buffer.Get(), 0, 
			resourceData.m_meshPrimStaging.m_validDataIndex * sizeof(PrimitiveSurface));
		cmdList->CopyBufferRegion(pRenderer->m_roResources.m_LODBuffer.buffer.Get(), 0, resourceData.m_lodDataStaging.m_buffer.Get(), 0,
			resourceData.m_lodDataStaging.m_validDataIndex * sizeof(LodData));
		cmdList->CopyBufferRegion(pRenderer->m_roResources.m_renderBuffer.buffer.Get(), BLIT_MAX_WORLD_VARIABLE_COUNT * sizeof(RenderObject), 
			instanceData.m_renderStaging.m_buffer.Get(), 0, instanceData.m_renderStaging.m_validDataIndex * sizeof(RenderObject));
		cmdList->CopyBufferRegion(pRenderer->m_roResources.m_renderBuffer.buffer.Get(), 0, instanceData.m_dynamicRenderStaging.m_buffer.Get(), 0,
			instanceData.m_dynamicRenderStaging.m_dataSize * sizeof(RenderObject));
		cmdList->CopyBufferRegion(pRenderer->MCpuLogicBuffers.GPUSSBOWorldVariableTransform.buffer.Get(), 0, instanceData.m_cpuTransformStaging.m_buffer.Get(), 0,
			instanceData.m_cpuTransformStaging.m_validDataIndex * sizeof(WVTransform));
		for (uint32_t frame = 0; frame < BlitzenDX12::ce_framesInFlight; frame++)
		{
			cmdList->CopyBufferRegion(pRenderer->m_rwResources[frame].m_transformBuffer.buffer.Get(), 0, instanceData.m_transformStaging.m_buffer.Get(), 0,
				instanceData.m_transformStaging.m_validDataIndex * sizeof(MeshTransform));
		}
		cmdList->CopyBufferRegion(pRenderer->m_roResources.m_boundingSpheres.buffer.Get(), 0, instanceData.m_boundingSpheresStaging.m_buffer.Get(), 0,
			instanceData.m_boundingSpheresStaging.m_validDataIndex * sizeof(BoundingSphere));

		cmdList->Close();
		ID3D12CommandList* commandLists[] = { cmdList };
		pRenderer->m_transferCommandQueue->ExecuteCommandLists(1, commandLists);

		BlitzenDX12::PlaceFence(pRenderer->m_cmdContext[0].m_copyFence.m_value, pRenderer->m_transferCommandQueue.Get(), pRenderer->m_cmdContext[0].m_copyFence.m_dx12Handle.Get(), 
			pRenderer->m_cmdContext[0].m_copyFence.m_event);


		return 1;
	}

	uint8_t UploadRendererIdleWorkResources(BlitzenDX12::Dx12Renderer* pRenderer, RENDERER_IDLE_MODE mode)
	{
		switch (mode)
		{
		case RENDERER_IDLE_MODE::TRIANGLE:
		{
			return 1;
		}
		case RENDERER_IDLE_MODE::BLITZEN_LOGO:
		{
			if (!BlitzenDX12::AddBlitzenLogoDescriptor(pRenderer->m_device.Get(), pRenderer->m_roResources, pRenderer->m_descriptorContext))
			{
				BLIT_ERROR("%s: Failed to add descriptors for Blitzen Logo Display during idle work", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!BlitzenDX12::CreateBlitzenLogoPipeline(pRenderer->m_device.Get(), pRenderer->m_pipelineContext))
			{
				BLIT_ERROR("%s: Failed to Create Pipeline for Blitzen Logo Display during idle work", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			auto cmdList = pRenderer->m_cmdContext[0].m_graphicsCmdList.Get();
			pRenderer->m_cmdContext[0].m_graphicsCmdAlloc->Reset();
			cmdList->Reset(pRenderer->m_cmdContext[0].m_graphicsCmdAlloc.Get(), nullptr);

			D3D12_RESOURCE_BARRIER textureTransitions{};
			BlitzenDX12::CreateResourcesTransitionBarrier(textureTransitions, pRenderer->m_roResources.m_drawTextures[0].resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			cmdList->ResourceBarrier(1, &textureTransitions);

			cmdList->Close();
			ID3D12CommandList* commandLists[] = { cmdList };
			pRenderer->m_commandQueue->ExecuteCommandLists(1, commandLists);

			BlitzenDX12::PlaceFence(pRenderer->m_cmdContext[0].m_frameFence.m_value, pRenderer->m_commandQueue.Get(), pRenderer->m_cmdContext[0].m_frameFence.m_dx12Handle.Get(), 
				pRenderer->m_cmdContext[0].m_frameFence.m_event);

			return 1;
		}
		default:
		{
			BLIT_ERROR("%s: Unhandled case statement enountered while Setting up Blitzen Logo pipeline", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}
		}
	}
}

#endif