#pragma once
#include "Renderer/BlitzenVulkan/Context/vulkanRenderer.h"
#include "Renderer/BlitzenGL/openglRenderer.h"
#include "Renderer/BlitzenDX12/Context/dx12Renderer.h"
#include "BlitCL/blitSmartPointer.h"

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

    uint8_t StartupRenderer(RendererPtrType pRenderer, uint32_t windowWidth, uint32_t windowHeight, BlitzenPlatform::PlatformContext* pPlatform);

    uint8_t UploadResourcesToGPU(RendererPtrType pRenderer, DrawContext& drawContext);

    uint8_t UploadTextureToGPU(RendererPtrType pRenderer, void* pTextureData);

    void PresentRender(RendererPtrType pRenderer, uint32_t waitCount);

    void PrepareRendererForRuntime(RendererPtrType pRenderer);

    void* GetMovingObjectsBufferMappedPointer(RendererPtrType pRenderer);

    void BarRenderFrame(RendererPtrType pContext);
    
    void GenerateHI_Z_MAP(RendererPtrType pContext);

    enum class BLIT_CULL_TYPE : uint8_t
    {
        NO_CULL,
        DRAW_CULL_DEFAULT,
        DRAW_CULL_INSTANCED,
        DRAW_CULL_TEMPORAL_OCCLUSION,
        CLUSTER_CULL_DEFAULT
    };
    struct CULL_CONTEXT
    {
        WORLD_RESIDENTS* m_pResidents{ nullptr };
        BLIT_CULL_TYPE m_cullType{ BLIT_CULL_TYPE::NO_CULL };
        RENDER_OBJECT_TYPE m_workType{ RENDER_OBJECT_TYPE::OPAQUE_STATIC };
        uint32_t m_workCount{ 0 };
    };
    void DispatchCullingShaders(RendererPtrType pContext, const CULL_CONTEXT& cullContext);

    //void UpdateRendererTransforms(RendererPtrType pContext, BlitzenCore::ARRAY_OF_POINTERS<DynamicTransform> pDynamicTransformArr, uint32_t dynamicTransformCount, MeshTransform* transformArr);

    void RenderObjects(RendererPtrType pContext, uint32_t renderOffset, RENDER_OBJECT_TYPE objectType, DrawContext& drawContext);

    BlitML::vec2 UpdateRendererWindowData(RendererPtrType pRenderer, uint32_t newWidth, uint32_t newHeight, BlitzenPlatform::PlatformContext* pPlatform);
}