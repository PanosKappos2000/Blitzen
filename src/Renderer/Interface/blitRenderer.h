#pragma once
#include "Renderer/BlitzenVulkan/vulkanRenderer.h"
#include "Renderer/BlitzenGL/openglRenderer.h"
#include "Renderer/BlitzenDX12/dx12Renderer.h"
#include "Renderer/Entities/DynamicTransform/blitDynamicTransform.h"

namespace BlitzenEngine
{
    #if defined(linux)
        using Renderer = BlitCL::SmartPointer<BlitzenVulkan::VulkanRenderer, BlitzenCore::AllocationType::Renderer>;

        using RendererPtrType = BlitzenVulkan::VulkanRenderer*;

        using RendererType = BlitzenVulkan::VulkanRenderer;

    #elif defined(_WIN32) && defined(BLIT_VK_FORCE)

        using Renderer = BlitCL::SmartPointer<BlitzenVulkan::VulkanRenderer, BlitzenCore::AllocationType::Renderer>;

        using RendererPtrType = BlitzenVulkan::VulkanRenderer*;

        using RendererType = BlitzenVulkan::VulkanRenderer;

    #elif defined(_WIN32) && defined(BLIT_GL_LEGACY_OVERRIDE) 

        using Renderer = BlitCL::SmartPointer<BlitzenGL::OpenglRenderer, BlitzenCore::AllocationType::Renderer>;

        using RendererPtrType = BlitzenGL::OpenglRenderer*;

		using RendererType = BlitzenGL::OpenglRenderer;

    #elif defined(_WIN32)

        using Renderer = BlitCL::SmartPointer<BlitzenDX12::Dx12Renderer, BlitzenCore::AllocationType::Renderer>;

        using RendererPtrType = BlitzenDX12::Dx12Renderer*;

        using RendererType = BlitzenDX12::Dx12Renderer;

    #else

        static_assert(true);

    #endif
    
    enum class BLIT_CULL_TYPE : uint8_t
    {
        NO_CULL,
        DRAW_CULL_DEFAULT,
        DRAW_CULL_INSTANCED,
        DRAW_CULL_TEMPORAL_OCCLUSION,
        CLUSTER_CULL_DEFAULT
    };
    
    void Init(RendererPtrType pContext);

    void BarRenderFrame(RendererPtrType pContext);

    void HI_Z_MAP_Gen(RendererPtrType pContext);

    void DispatchCullingShaders(RendererPtrType pContext, uint32_t workCount, uint32_t workOffset, BLIT_CULL_TYPE cullingFlags, RENDER_OBJECT_TYPE objectType);

    void UpdateRendererTransforms(RendererPtrType pContext, BlitzenCore::ARRAY_OF_POINTERS<DynamicTransform> pDynamicTransformArr, uint32_t dynamicTransformCount, MeshTransform* transformArr);

    void RenderObjects(RendererPtrType pContext, uint32_t renderOffset, RENDER_OBJECT_TYPE objectType, DrawContext& drawContext);

    bool RenderingResourcesInit(RenderingResources* pResources, RendererPtrType pRenderer);
}