#pragma once 

#include "vulkanData.h"

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
}