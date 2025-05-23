#pragma once
#include "Renderer/Resources/Scene/blitScene.h"
#include "Core/Entities/blitEntityManager.h"
#include "Renderer/BlitzenVulkan/vulkanRenderer.h"
#if defined(_WIN32)
    #include "Renderer/BlitzenGl/openglRenderer.h"
#endif
#if defined(_WIN32)
    #include "Renderer/BlitzenDX12/dx12Renderer.h"
#endif
#include <typeinfo>
#include <cstring>

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

    void UpdateRendererTransform(RendererPtrType pRenderer, RendererTransformUpdateContext& context);

    bool RenderingResourcesInit(RenderingResources* pResources, RendererPtrType pRenderer);

    bool ManageGltf(const char* filepath, RenderingResources* pResources, BlitzenCore::EntityManager* pManager, RendererPtrType pRenderer);

    void CreateDynamicObjectRendererTest(BlitzenEngine::RenderContainer& renders, BlitzenEngine::MeshResources& meshes, BlitzenCore::EntityManager* pManager);

    bool CreateSceneFromArguments(int argc, char** argv, BlitzenEngine::RenderingResources* pResources, BlitzenEngine::RendererPtrType pRenderer, BlitzenCore::EntityManager* pManager);

    void UpdateDynamicObjects(RendererPtrType pRenderer, BlitzenCore::EntityManager* pEntityManager, BlitzenWorld::BlitzenWorldContext& blitzenContext);    
}