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

        VkWriteDescriptorSet m_pushDescriptorsClusterCull[Ce_ClusterCullDescriptorCount];

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

        VkDescriptorBufferInfo m_drawVisDescInfo[ce_framesInFlight]{};

        VkDescriptorBufferInfo m_vtxDescInfo{};

        VkDescriptorBufferInfo m_matDescInfo{};

        VkDescriptorBufferInfo m_clusterBufferDescInfo{};

        VkDescriptorBufferInfo m_renderBufferDescInfo[ce_framesInFlight]{};


        DescriptorSetLayout m_pushDescriptorLayout;

        DescriptorSetLayout m_HI_Z_descriptorSetLayout;

        DescriptorSetLayout m_textureDescriptorSetlayout;

        DescriptorSetLayout m_backgroundSetLayout;

        DescriptorSetLayout m_presentSetlayout;
    };

    struct RWResources
    {
        BlitVk_CPU_DATA_SSBO<BlitzenEngine::MeshTransform> m_transformBuffer;

        BlitVk_UBUFFER<BlitzenEngine::CameraViewData> m_viewDataBuffer;

        BlitVk_SSBO m_drawCmdBuffer;

        BlitVk_SSBO m_drawCmdCounterBuffer;

        BlitVk_SSBO m_clusterGroupDataBuffer;

        BlitVk_SSBO m_clusterDispatchCounterBuffer;

        BlitVk_UBUFFER<uint32_t> m_clusterDispatchCounterCopy;

        BlitVk_SSBO m_transClusterGroupDataBuffer;

        BlitVk_SSBO m_transClusterDispatchCounterBuffer;

        BlitVk_UBUFFER<uint32_t> m_transClusterDispatchCounterCopy;

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

        BlitVk_SSBO m_vtxBuffer;

        BlitVk_SSBO m_idxBuffer;

        BlitVk_SSBO m_renderBuffer;

        BlitVk_SSBO m_transRenderBuffer;

        BlitVk_SSBO m_surfaceBuffer;

        BlitVk_SSBO m_LODBuffer;

        BlitVk_SSBO m_matBuffer;

        BlitVk_SSBO m_clusterBuffer;

        BlitVk_SSBO m_clusterIdxBuffer;

        BlitVk_SSBO m_blas;
        AccelerationStructure m_blasData[BlitzenCore::Ce_MaxMeshPrimitivesCount];
        BlitVk_SSBO m_tlas;
        AccelerationStructure m_tlasData;
    };
}