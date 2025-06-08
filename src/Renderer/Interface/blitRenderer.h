#pragma once
#include "Renderer/Resources/Scene/blitScene.h"
#include "Renderer/Entities/blitEntityManager.h"
#include "Renderer/BlitzenVulkan/vulkanRenderer.h"
#include "Renderer/BlitzenGL/openglRenderer.h"
#include "Renderer/BlitzenDX12/dx12Renderer.h"
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

    struct WORLD_blit
    {
        BlitCL::DynamicArray<SceneContext> m_scenes;

        Renderer P_RENDERER;

        DrawContext m_drawContext;

        inline WORLD_blit(Camera& camera, MeshResources& meshes, RenderContainer& renders, TextureManager& textureManager, BlitzenPlatform::PlatformContext* pPlatform)
            :m_drawContext{ camera, meshes, renders, textureManager, pPlatform }
        {

        }
    };

    bool RenderingResourcesInit(RenderingResources* pResources, RendererPtrType pRenderer);

    bool ManageGltf(const char* filepath, RenderingResources* pResources, EntityManager* pManager, RendererPtrType pRenderer, SceneContext* pScene = nullptr);

    void CreateDynamicObjectRendererTest(RenderContainer& renders, MeshResources& meshes, EntityManager* pManager, SceneContext* pScene = nullptr);

    void LoadGeometryStressTest(RenderContainer& renders, MeshResources& meshContext, float transformMultiplier, SceneContext* pScene = nullptr);

    bool CreateSceneFromArguments(int argc, char** argv, RenderingResources* pResources, WORLD_blit* pWORLD, EntityManager* pManager);

    void UpdateDynamicObjects(RendererPtrType pRenderer, EntityManager* pEntityManager, BlitzenWorld::BlitzenWorldContext& blitzenContext);    
}