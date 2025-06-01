#pragma once 

#include "vulkanData.h"
#include "Game/blitCamera.h"

namespace BlitzenVulkan
{
	struct PipelineContext
	{
        PipelineLayout m_hiZLayout;
        PipelineObject m_hiZPso;

        // Culling shaders. Initial does furstum culling and LOD selection
        PipelineLayout m_drawCullLayout;

        PipelineObject m_drawCullFirstPso;
        PipelineObject m_drawCullLatePso;
        PipelineObject m_transDrawCullPso;


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
	};

    struct DescriptorContext
    {

        VkWriteDescriptorSet m_pushDescriptorsShared[Ce_SharedDescriptorCount]{};

        VkWriteDescriptorSet m_pushDescriptorsCull[Ce_CullDescriptorCount]{};

        VkWriteDescriptorSet m_drawCullDescriptors[Ce_GraphicsDescriptorCount]{};

        DescriptorSetLayout m_pushDescriptorLayout;
        
        DescriptorSetLayout m_depthPyramidDescriptorLayout;

        DescriptorSetLayout m_textureDescriptorSetlayout;

        DescriptorPool m_textureDescriptorPool;
        VkDescriptorSet m_textureDescriptorSet;

        DescriptorSetLayout m_backgroundImageSetLayout;

        DescriptorSetLayout m_generatePresentationImageSetLayout;
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
    };

    struct ROResources
    {

    };
}