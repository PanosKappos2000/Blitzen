#pragma once 
#include "vulkanData.h"
#include "Renderer/View/blitCamera.h"

namespace BlitzenVulkan
{

    struct CommandContext
    {
        CommandPool m_mainGraphicsCmdPool;
        VkCommandBuffer m_mainGraphicsCmdB;

        CommandPool m_transferCmdPool;
        VkCommandBuffer m_transferCmdB;

        CommandPool m_computeCmdPool;
        VkCommandBuffer m_computeCmdB;

        CommandPool m_uiGraphicsCmdPool;
        VkCommandBuffer m_uiGraphicsCmdBuffer;

        SyncFence m_preClusterFence;
        SyncFence m_frameFence;
        SyncFence m_uiFence;

        Semaphore m_swapchainSemaphore;
        Semaphore m_bufferUpdateSemaphore;
        Semaphore m_renderSemaphore;
        Semaphore m_dasherRenderSemaphore;

        Semaphore m_clusterSemaphore;

        uint8_t Init(VkDevice device, Queue graphicsQueue, Queue transferQueue, Queue computeQueue);
    };

	struct PipelineContext
	{
        PipelineLayout m_hiZLayout;
        PipelineObject m_hiZPso;

        // Culling shaders. Initial does furstum culling and LOD selection
        PipelineLayout m_drawCullLayout;

        PipelineObject m_drawCullFirstPso;
        PipelineObject m_drawCullLatePso;
        PipelineObject m_transDrawCullPso;

        // Temporal occlusion mode
        PipelineObject m_drawTemporalOccPso;

        // CLUSTER MODE
        PipelineLayout m_clusterCullLayout;

        PipelineObject m_clusterCullDispatchPso;
        PipelineObject m_clusterCullPso;


        // Main graphics pipeline. Draws opaque objects that have no special properties
        PipelineLayout m_opaqueDrawLayout;

        PipelineObject m_opaqueDrawPso;
        PipelineObject m_tranparentDrawPso;
        
        // Present
        PipelineObject m_presentPso;
        PipelineLayout m_presentLayout;

        // Background
        PipelineObject m_backgroundPso;
        PipelineLayout m_backgroundLayout;

        // Triangle loading screen
        PipelineObject m_trianglePso;

        PipelineLayout m_triangleLayout;
        BlitML::vec3 m_loadingTriangleVertexColor;

        // Draws objects that use the near plane clipping matrix.
        PipelineObject m_onpcDraw;
        PipelineLayout m_onpcLayout;

        // Oblique Near place clipping culling pipeline. Might not be necessary
        PipelineObject m_onpcCull;

        VkRenderingAttachmentInfo m_depthTargetInfo[ce_framesInFlight]{};
        VkRenderingAttachmentInfo m_colorTargetInfo[ce_framesInFlight]{};
	};

    struct DescriptorContext
    {
        VkWriteDescriptorSet m_pushDescriptorsShared[Ce_SharedDescriptorCount * ce_framesInFlight]{};

        VkWriteDescriptorSet m_pushDescriptorsCull[Ce_CullDescriptorCount * ce_framesInFlight]{};

        VkWriteDescriptorSet m_pushDescriptorsGraphics[Ce_GraphicsDescriptorCount * ce_framesInFlight]{};

        VkWriteDescriptorSet m_pushDescriptorsDrawOcc[Ce_DrawOcclusionDescriptorCount * ce_framesInFlight]{};

        VkWriteDescriptorSet m_pushDescriptorsClusterCull[Ce_ClusterCullDescriptorCount * ce_framesInFlight];

        VkWriteDescriptorSet m_HI_Z_cullDescriptor[ce_framesInFlight]{};

        VkWriteDescriptorSet m_colorTargetDescriptor[ce_framesInFlight]{};

        VkWriteDescriptorSet m_HI_Z_descriptors[ce_framesInFlight * 2]{};


        VkDescriptorSet m_textureDescriptorSet{};
        DescriptorPool m_textureDescriptorPool;


        VkDescriptorImageInfo m_colorTargetDescInfo[ce_framesInFlight]{};

        VkDescriptorImageInfo m_depthTargetDescInfo[ce_framesInFlight]{};

        VkDescriptorImageInfo m_HI_Z_descInfo[ce_framesInFlight]{};

        VkDescriptorBufferInfo m_viewDescInfo[ce_framesInFlight]{};

        VkDescriptorBufferInfo m_surfaceDescInfo[ce_framesInFlight]{};

        VkDescriptorBufferInfo m_drawCmdDescInfo[ce_framesInFlight]{};

        VkDescriptorBufferInfo m_transformDescInfo[ce_framesInFlight]{};

        VkDescriptorBufferInfo m_LODDescInfo[ce_framesInFlight]{};

        VkDescriptorBufferInfo m_drawCmdCounterDescInfo[ce_framesInFlight]{};

        VkDescriptorBufferInfo m_boundingSphereDescInfo[ce_framesInFlight]{};

        VkDescriptorBufferInfo m_drawVisDescInfo[ce_framesInFlight]{};

        VkDescriptorBufferInfo m_vtxPosDescInfo{};
        VkDescriptorBufferInfo m_vtxNrmDescInfo{};
        VkDescriptorBufferInfo m_vtxTngDescInfo{};
        VkDescriptorBufferInfo m_vtxTexCoordsInfo{};

        VkDescriptorBufferInfo m_matDescInfo{};

        VkDescriptorBufferInfo m_clusterBufferDescInfo{};
        VkDescriptorBufferInfo m_clusterGroupDescInfo{};
        VkDescriptorBufferInfo m_clusterGroupCounterDescInfo{};

        VkDescriptorBufferInfo m_renderBufferDescInfo[ce_framesInFlight]{};

        DescriptorSetLayout m_pushDescriptorLayout;

        DescriptorSetLayout m_HI_Z_descriptorSetLayout;

        DescriptorSetLayout m_textureDescriptorSetlayout;

        DescriptorSetLayout m_backgroundSetLayout;

        DescriptorSetLayout m_presentSetlayout;
    };

    struct RWResources
    {
        BlitVk_SSBO m_transformBuffer;

        BlitVk_UBUFFER<BlitzenEngine::CameraViewData> m_viewDataBuffer;

        BlitVk_SSBO m_staticDrawCmdBuffer;
        BlitVk_SSBO m_staticDrawCmdCount;

        BlitVk_SSBO m_movementBuffer;
        BlitVk_SSBO m_dynamicDrawCmdBuffer;
        BlitVk_SSBO m_dynamicDrawCmdCounter;

        BlitVk_SSBO m_clusterGroupDataBuffer;
        BlitVk_SSBO m_clusterGroupCounter;
        BlitVk_SSBO m_clusterVisibilityBuffer;
        BlitVk_SSBO m_clusterDrawCmdBuffer;
        BlitVk_SSBO m_clusterDrawCounter;

        BlitVk_SSBO m_instanceDrawCmdBuffer;

        BlitVk_SSBO m_drawVisBuffer;

        BlitVk_2DIMAGE_SAMP m_colorTarget;

        BlitVk_2DIMAGE_SAMP m_depthTarget;

        HI_Z_MAP m_HI_Z_MAP;
    };

    struct ROResources
    {
        TextureData m_textures[BlitzenCore::Ce_MaxTextureCount];
        uint32_t m_textureCount;

        ImageSampler m_textureSampler;

        BlitVk_SSBO m_vtxPosBuffer;
        BlitVk_SSBO m_vtxNrmBuffer;
        BlitVk_SSBO m_vtxTngBuffer;
        BlitVk_SSBO m_vtxTexCoordBuffer;

        BlitVk_SSBO m_idxBuffer;

        BlitVk_SSBO m_clusterVtxsBuffer{};
        BlitVk_SSBO m_clusterSpheresBuffer{};
        BlitVk_SSBO m_clusterConesBuffer{};

        BlitVk_SSBO m_clusterIdxBuffer;

        BlitVk_SSBO m_terrainVtxBuffer{};
        BlitVk_SSBO m_terrainIdxBuffer{};
        BlitVk_SSBO m_terrainHeightBuffer{};

        BlitVk_SSBO m_renderBuffer;
        BlitVk_SSBO m_boundingSphereBuffer;
        BlitVk_SSBO m_surfaceBuffer;
        BlitVk_SSBO m_LODBuffer;
        BlitVk_SSBO m_matBuffer;

        BlitVk_SSBO m_blas;
        AccelerationStructure m_blasData[BlitzenCore::Ce_MaxMeshPrimitivesCount];
        BlitVk_SSBO m_tlas;
        AccelerationStructure m_tlasData;

        BlitVk_STAGING<BlitzenEngine::CPU_TRANSFORM> CPU_MOVING_RESIDENTS_MAPPED{};
        BlitVk_STAGING<BlitzenEngine::CPU_TRANSFORM> GPU_MOVING_OBJECT_READBACK{};

        BlitVk_STAGING<BlitzenEngine::GridCellOffsets> GPU_GRID_CELL_OFFSETS_READBACK{};
        BlitVk_STAGING<uint32_t> GPU_GRID_COLLIDER_INDICES_READBACK{};
    };

    struct LoadingContextMesh
    {
        BlitVk_STAGING<BlitzenEngine::VtxPos> m_vtxPosStaging;
        BlitVk_STAGING<BlitzenEngine::VtxNormals> m_vtxNrmStaging;
        BlitVk_STAGING<BlitzenEngine::VtxTangents> m_vtxTngStaging;
        BlitVk_STAGING<BlitzenEngine::VtxTexCoords> m_vtxTexCoordStaging;
        BlitVk_STAGING<uint32_t> m_vtxIdxStaging;
        BlitVk_STAGING<BlitzenEngine::ClusterVertices> m_clusterVtxStaging;
        BlitVk_STAGING<BlitzenEngine::ClusterSphere> m_clusterSpheresStaging;
        BlitVk_STAGING<BlitzenEngine::ClusterCone> m_clusterConesStaging;
        BlitVk_STAGING<uint32_t> m_clusterIdxStaging;
        BlitVk_STAGING<BlitzenEngine::PrimitiveSurface> m_meshPrimStaging;
        BlitVk_STAGING<BlitzenEngine::LodData> m_lodDataStaging;
    };

    struct LoadingContextRenderObjects
    {
        BlitVk_STAGING<BlitzenEngine::RenderObject> m_renderStaging;
        BlitVk_STAGING<BlitzenEngine::RenderObject> m_dynamicRenderStaging;
        BlitVk_STAGING<BlitzenEngine::MeshTransform> m_transformStaging;
        BlitVk_STAGING<BlitzenEngine::CPU_TRANSFORM> m_cpuTransformStaging;
        BlitVk_STAGING<BlitzenEngine::BoundingSphere> m_boundingSpheresStaging;
    };
}