#if defined(_WIN32)
#include "dx12ResourcesUpload.h"
#include "Renderer/Interface/blitRenderer.h"
#include "Renderer/BlitzenDX12/Resources/dx12RNDResources.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenEngine
{
	uint8_t UploadResourcesToGPU(BlitzenDX12::Dx12Renderer* pRenderer, DrawContext& drawContext)
	{
		if (!BlitzenEngine::GenerateHlslVertices(drawContext.m_meshes.m_triangles))
		{
			BLIT_ERROR("%s: Failed to generate HLSL vertices", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			if (!BlitzenEngine::GenerateHLSLClusters(drawContext.m_meshes.m_clusters))
			{
				BLIT_ERROR("%s: Failed to generate HLSL clusters", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}
		}

		if (!BlitzenDX12::UploadResourcesToBuffers(pRenderer->m_device.Get(), drawContext, pRenderer->m_roResources, pRenderer->m_rwResources, pRenderer->m_cmdContext[0], 
			pRenderer->m_transferCommandQueue.Get()))
		{
			BLIT_ERROR("%s: Failed to upload resources to GPU buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BlitzenDX12::CreateResourceViews(pRenderer->m_device.Get(), pRenderer->m_descriptorContext, pRenderer->m_cmdContext[pRenderer->m_currentFrame], pRenderer->m_transferCommandQueue.Get(), 
			pRenderer->m_roResources, pRenderer->m_rwResources, drawContext, pRenderer->m_depthBuffers, pRenderer->m_swapchainWidth, pRenderer->m_swapchainHeight);

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

	uint8_t AllocateLoadingContextMesh(BlitzenDX12::Dx12Renderer* pRenderer, BlitzenDX12::LoadingContextMesh& ctx)
	{
		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_meshPrimStaging, BlitzenCore::Ce_MaxMeshPrimitivesCount, (PrimitiveSurface*)nullptr))
		{
			BLIT_ERROR("%s: Failed to create mesh primitives staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_lodDataStaging, BlitzenCore::Ce_MaxMeshPrimitivesCount * BlitzenCore::Ce_MaxLodCountPerSurface, (LodData*)nullptr))
		{
			BLIT_ERROR("%s: Failed to create Mesh Primitives Level of Detail data staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_vtxPosStaging, BlitzenCore::Ce_MaxWorldVertexCount, (VtxPos*)nullptr))
		{
			BLIT_ERROR("%s: Failed to create Vertex Positions Staging Buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_vtxNrmStaging, BlitzenCore::Ce_MaxWorldVertexCount, (VtxNormals*)nullptr))
		{
			BLIT_ERROR("%s: Failed to create Vertex Normals Staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_vtxTngStaging, BlitzenCore::Ce_MaxWorldVertexCount, (VtxTangents*)nullptr))
		{
			BLIT_ERROR("%s: Failed to create Vertex Tangents Staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_vtxTexCoordStaging, BlitzenCore::Ce_MaxWorldVertexCount, (VtxTexCoords*)nullptr))
		{
			BLIT_ERROR("%s: Failed to create Vertex Texture Coordinates staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_vtxIdxStaging, BlitzenCore::Ce_MaxWorldVertexIndicesCount, (uint32_t*)nullptr))
		{
			BLIT_ERROR("%s: Failed to create Vertex Indices staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_clusterVtxStaging, CE_MAX_WORLD_CLUSTER_COUNT, (ClusterVertices*)nullptr))
			{
				BLIT_ERROR("%s: Failed to create Cluster Vertices staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_clusterSpheresStaging, CE_MAX_WORLD_CLUSTER_COUNT, (ClusterSphere*)nullptr))
			{
				BLIT_ERROR("%s: Failed to create Cluster Spheres staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_clusterConesStaging, CE_MAX_WORLD_CLUSTER_COUNT, (ClusterCone*)nullptr))
			{
				BLIT_ERROR("%s: Failed to create Cluster Cones staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!BlitzenDX12::CreateStaging(pRenderer->m_device.Get(), ctx.m_clusterIdxStaging, BlitzenCore::Ce_MaxWorldVertexIndicesCount, (uint32_t*)nullptr))
			{
				BLIT_ERROR("%s: Failed to create Cluster Indices staging buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
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

	uint8_t UploadToLODDataStagingBufer(BlitzenDX12::LoadingContextMesh& ctx, LodData* LODs, uint32_t count)
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
}

#endif