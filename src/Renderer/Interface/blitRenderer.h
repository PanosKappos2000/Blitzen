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
    
    // API initialization. Requests handles, preallocates buffers and checks that the host can succesfully run the chosen renderer
    uint8_t StartupRenderer(RendererPtrType pRenderer, uint32_t windowWidth, uint32_t windowHeight, BlitzenPlatform::PlatformContext* pPlatform);

    // Post-loading function. CPU side resources passed to GPU side buffers (or whichever type of handle is used)
    // Views for resources also placed
    uint8_t UploadResourcesToGPU(RendererPtrType pRenderer, DrawContext& drawContext);

    // Singular texture upload
    uint8_t UploadTextureToGPU(RendererPtrType pRenderer, void* pTextureData);

    // Returns pointer to the buffer which is responsible for dynamic transform data copy. 
    // If this is never called, the application will either brreak or dynamic objects will not move
    void* GetMovingObjectsBufferMappedPointer(RendererPtrType pRenderer);

    // Finalizes resources so that the renderer is ready for culling and rendering
    void PrepareRendererForRuntime(RendererPtrType pRenderer);

    enum class RENDERER_FENCE_TYPE : uint8_t
    {
        PREVIOUS_FRAME,
        BUFFER_UPDATE,
        CULL
    };
    // Custom CPU fence. Meant to stop renderer command recording until a desired event.
    void PlaceRendererFence(RendererPtrType pRenderer, RENDERER_FENCE_TYPE type);

    // Gives new camera values to the shader. Static object culling can be dispatched after this
    void UpdateRendererView(RendererPtrType pRenderer, CameraViewData& camera, bool isFrustumFrozen);
    
    // Generates a Hierarchical depth buffer by copying the depth target. Allow for occlusion culling. Should be called before transparent object are drawn
    void GenerateHI_Z_MAP(RendererPtrType pRenderer);

    // Should be called after game logic is done. Copies movement data to shader buffer. Dynamic object culling can be dispatched after this
    void UpdateRendererTransforms(RendererPtrType pRenderer);

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
    // Culls a group of renders using compute shader. Prepares draw commands for rendering
    void DispatchCullingShaders(RendererPtrType pRenderer, const CULL_CONTEXT& cullContext);

    // Needs to be called before the first render pass to define vieport
    void SetupForFirstRenderPass(RendererPtrType pRenderer);

    enum class BLIT_RENDER_TYPE : uint8_t
    {
        RENDER_OPAQUE,
        RENDER_INSTANCED,
        RENDER_TRANSPARENT
    };
    struct RENDER_CONTEXT
    {
        BLIT_RENDER_TYPE m_renderType{ BLIT_RENDER_TYPE::RENDER_OPAQUE };
    };
    void RenderObjects(RendererPtrType pRenderer, const RENDER_CONTEXT& renderContext);

    void FinalizeRendering(RendererPtrType pRenderer);

    void PresentRender(RendererPtrType pRenderer, uint32_t waitCount);

    // Should be called by event system to update renderer surface resources when a window resize event is encountered
    BlitML::vec2 UpdateRendererWindowData(RendererPtrType pRenderer, uint32_t newWidth, uint32_t newHeight, BlitzenPlatform::PlatformContext* pPlatform);
}