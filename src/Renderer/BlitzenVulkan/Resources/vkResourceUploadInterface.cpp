#include "Renderer/Interface/blitRenderer.h"
#include "vkResourcesUpload.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanRNDResources.h"

namespace BlitzenEngine
{
	uint8_t UploadResourcesToGPU(BlitzenVulkan::VulkanRenderer* pRenderer, BlitzenEngine::DrawContext& drawContext, BlitzenVulkan::LoadingContextMesh& loadingContextMesh)
	{
		if (!BlitzenVulkan::UploadResourcesToBuffers(pRenderer->m_device, pRenderer->m_instance, pRenderer->m_allocator, pRenderer->m_transferQueue.handle, drawContext, pRenderer->m_readOnlies,
			pRenderer->m_readWrites, pRenderer->m_commandsContext[0], pRenderer->m_stats, loadingContextMesh))
		{
			BLIT_ERROR("%s: Failed to upload data to buffers", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenVulkan::AllocateTextureDescriptorSet(pRenderer->m_device, pRenderer->m_readOnlies, pRenderer->m_descriptorContext))
		{
			BLIT_ERROR("Failed to allocate texture descriptor sets");
			return 0;
		}

		BlitzenVulkan::CreateDescriptors(pRenderer->m_descriptorContext, pRenderer->m_readOnlies, pRenderer->m_readWrites, drawContext);

		// Updates the reference to the depth pyramid width held by the camera
		drawContext.m_camera.viewData.pyramidWidth = float(pRenderer->m_readWrites[0].m_HI_Z_MAP.m_pyramid.m_width);
		drawContext.m_camera.viewData.pyramidHeight = float(pRenderer->m_readWrites[0].m_HI_Z_MAP.m_pyramid.m_height);

		return 1;
	}

	uint8_t AllocateLoadingContextMesh(BlitzenVulkan::VulkanRenderer* pRenderer, BlitzenVulkan::LoadingContextMesh& ctx)
	{
		if (!BlitzenVulkan::CreateEmptyStaging(pRenderer->m_allocator, pRenderer->m_device, ctx.m_meshPrimStaging, BlitzenCore::Ce_MaxMeshPrimitivesCount))
		{
			BLIT_FATAL("%s: Failed to create mesh primitives staging buffer", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenVulkan::CreateEmptyStaging(pRenderer->m_allocator, pRenderer->m_device, ctx.m_lodDataStaging, BlitzenCore::Ce_MaxMeshPrimitivesCount * BlitzenCore::Ce_MaxLodCountPerSurface))
		{
			BLIT_FATAL("%s: Failed to create Mesh Primitives Level of Detail data staging buffer", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenVulkan::CreateEmptyStaging(pRenderer->m_allocator, pRenderer->m_device, ctx.m_vtxPosStaging, BlitzenCore::Ce_MaxWorldVertexCount))
		{
			BLIT_FATAL("%s: Failed to create Vertex Positions Staging Buffer", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenVulkan::CreateEmptyStaging(pRenderer->m_allocator, pRenderer->m_device, ctx.m_vtxNrmStaging, BlitzenCore::Ce_MaxWorldVertexCount))
		{
			BLIT_FATAL("%s: Failed to create Vertex Normals Staging buffer", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenVulkan::CreateEmptyStaging(pRenderer->m_allocator, pRenderer->m_device, ctx.m_vtxTngStaging, BlitzenCore::Ce_MaxWorldVertexCount))
		{
			BLIT_FATAL("%s: Failed to create Vertex Tangents Staging buffer", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenVulkan::CreateEmptyStaging(pRenderer->m_allocator, pRenderer->m_device, ctx.m_vtxTexCoordStaging, BlitzenCore::Ce_MaxWorldVertexCount))
		{
			BLIT_FATAL("%s: Failed to create Vertex Texture Coordinates staging buffer", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
			return 0;
		}

		if (!BlitzenVulkan::CreateEmptyStaging(pRenderer->m_allocator, pRenderer->m_device, ctx.m_vtxIdxStaging, BlitzenCore::Ce_MaxWorldVertexIndicesCount))
		{
			BLIT_FATAL("%s: Failed to create Vertex Indices staging buffer", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
			return 0;
		}

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			if (!BlitzenVulkan::CreateEmptyStaging(pRenderer->m_allocator, pRenderer->m_device, ctx.m_clusterVtxStaging, CE_MAX_WORLD_CLUSTER_COUNT))
			{
				BLIT_FATAL("%s: Failed to create Cluster Vertices staging buffer", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
				return 0;
			}

			if (!BlitzenVulkan::CreateEmptyStaging(pRenderer->m_allocator, pRenderer->m_device, ctx.m_clusterSpheresStaging, CE_MAX_WORLD_CLUSTER_COUNT))
			{
				BLIT_FATAL("%s: Failed to create Cluster Spheres staging buffer", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
				return 0;
			}

			if (!BlitzenVulkan::CreateEmptyStaging(pRenderer->m_allocator, pRenderer->m_device, ctx.m_clusterConesStaging, CE_MAX_WORLD_CLUSTER_COUNT))
			{
				BLIT_FATAL("%s: Failed to create Cluster Cones staging buffer", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
				return 0;
			}

			if (!BlitzenVulkan::CreateEmptyStaging(pRenderer->m_allocator, pRenderer->m_device, ctx.m_clusterIdxStaging, BlitzenCore::Ce_MaxWorldVertexIndicesCount))
			{
				BLIT_FATAL("%s: Failed to create Cluster Indices staging buffer", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
				return 0;
			}
		}

		// Success
		return 1;
	}

	uint8_t UploadToMeshPrimitiveStagingBuffer(BlitzenVulkan::LoadingContextMesh& ctx, PrimitiveSurface* primitives, uint32_t count)
	{
		return 1;
	}

	uint8_t UploadToLODDataStagingBuffer(BlitzenVulkan::LoadingContextMesh& ctx, LodData* LODs, uint32_t count)
	{
		return 1;
	}

	uint8_t UploadToVertexPositionsStagingBuffer(BlitzenVulkan::LoadingContextMesh& ctx, VtxPos* vtxPositions, uint32_t count)
	{
		return 1;
	}

	uint8_t UploadToVertexNormalsStagingBuffer(BlitzenVulkan::LoadingContextMesh& ctx, VtxNormals* vtxNormals, uint32_t count)
	{
		return 1;
	}

	uint8_t UploadToVertexTangentsStagingBuffer(BlitzenVulkan::LoadingContextMesh& ctx, VtxTangents* vtxTangents, uint32_t count)
	{
		return 1;
	}

	uint8_t UploadToVertexTextureCoordinatesStagingBuffer(BlitzenVulkan::LoadingContextMesh& ctx, VtxTexCoords* vtxTexCoords, uint32_t count)
	{
		return 1;
	}

	uint8_t UploadToVertexIndicesStagingBuffer(BlitzenVulkan::LoadingContextMesh& ctx, uint32_t* indices, uint32_t count)
	{
		return 1;
	}

	uint8_t UploadToClusterVerticesStagingBuffer(BlitzenVulkan::LoadingContextMesh& ctx, ClusterVertices* clusterVertices, uint32_t count)
	{
		return 1;
	}
	
	uint8_t UploadToClusterSpheresStagingBuffer(BlitzenVulkan::LoadingContextMesh& ctx, ClusterSphere* clusterSpheres, uint32_t count)
	{
		return 1;
	}
	
	uint8_t UploadToClusterConesStagingBuffer(BlitzenVulkan::LoadingContextMesh& ctx, ClusterCone* clusterCones, uint32_t count)
	{
		return 1;
	}

	uint8_t UploadToClusterIndicesStagingBuffer(BlitzenVulkan::LoadingContextMesh& ctx, uint32_t* clusterIndices, uint32_t count)
	{
		return 1;
	}

	uint8_t AllocateLoadingContextRenderObjects(BlitzenVulkan::VulkanRenderer* pRenderer, BlitzenVulkan::LoadingContextMesh& ctx)
	{
		return 1;
	}

	uint8_t UploadToRenderObjectStagingBuffer(BlitzenVulkan::LoadingContextMesh& ctx, RenderObject* renderObjects, uint32_t renderCount)
	{
		return 1;
	}

	uint8_t UploadToDynamicRenderObjectStagingBuffer(BlitzenVulkan::LoadingContextMesh& ctx, RenderObject* renderObjects, uint32_t renderCount)
	{
		return 1;
	}

	uint8_t UploadToWorldTransformStagingBuffer(BlitzenVulkan::LoadingContextMesh& ctx, MeshTransform* transforms, uint32_t transformCount)
	{
		return 1;
	}

	uint8_t UploadToCPUTransformStagingBuffer(BlitzenVulkan::LoadingContextMesh& ctx, CPU_TRANSFORM* transforms, uint32_t transformCount)
	{
		return 1;
	}

	uint8_t UploadToRenderObjectStagingBuffer_MKII(BlitzenVulkan::VulkanRenderer* pRenderer, BlitzenVulkan::LoadingContextMesh& ctx, RenderObject* renderObjects, uint32_t renderCount)
	{
		return 1;
	}

	uint8_t UploadToDynamicRenderObjectStagingBuffer_MKII(BlitzenVulkan::VulkanRenderer* pRenderer, BlitzenVulkan::LoadingContextMesh& ctx, RenderObject* renderObjects, uint32_t renderCount)
	{
		return 1;
	}

	uint8_t UploadToWorldTransformStagingBuffer_MKII(BlitzenVulkan::VulkanRenderer* pRenderer, BlitzenVulkan::LoadingContextMesh& ctx, MeshTransform* transforms, uint32_t transformCount)
	{
		return 1;
	}

	uint8_t UploadToCPUTransformStagingBuffer_MKII(BlitzenVulkan::VulkanRenderer* pRenderer, BlitzenVulkan::LoadingContextMesh& ctx, CPU_TRANSFORM* transforms, uint32_t transformCount)
	{
		return 1;
	}

	uint8_t UploadToBoundingSphereStagingBuffer_MKII(BlitzenVulkan::VulkanRenderer* pRenderer, BlitzenVulkan::LoadingContextMesh& ctx, BoundingSphere* boundingSpheres, uint32_t sphereCount)
	{
		return 1;
	}

	uint8_t UploadNewGeometryDataToSSBOs(BlitzenVulkan::VulkanRenderer* pRenderer, BlitzenVulkan::LoadingContextMesh& instanceData, RenderingLoadingContextMesh& resourceData)
	{
		return 1;
	}
}