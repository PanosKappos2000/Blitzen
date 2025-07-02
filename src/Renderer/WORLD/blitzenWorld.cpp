#include "blitzenWorld.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenWorld
{
    inline WORLD_blit* p_BLITZEN_WORLD = nullptr;

    void WORLD_blit::DispatchFrameEvents(float deltaTime)
    {
        for (uint32_t event = 0; event < m_frameEvents.m_frameEventCount; ++event)
        {
            auto& frameEvent = m_frameEvents.m_frameEvents[event];
            frameEvent.m_function(frameEvent.m_worldVariableArg, deltaTime);
        }
    }

    bool RenderingResourcesInit(BlitzenEngine::RenderingResources* pResources, BlitzenEngine::RendererPtrType pRenderer)
    {
        pResources->m_textureManager.ALLOC();
        if (!BlitzenEngine::UploadTextureToGPU(pRenderer, pResources->m_textureManager.m_singleTextureHandle, "Assets/Textures/BlitzenLSV1.dds"))
        {
            BLIT_ERROR("%s: Rendering resources failed", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        if (!BlitzenEngine::UploadRendererIdleWorkResources(pRenderer, BlitzenEngine::RENDERER_IDLE_MODE::BLITZEN_LOGO))
        {
            BLIT_ERROR("%s: Failed to put renderer on Idle Work Mode", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        // Does not return false by design, might change later.
        if (!pResources->m_textureManager.AddTexture(BlitzenCore::Ce_DefaultTextureName))
        {
            BLIT_ERROR("%s: Something went wrong with texture map", BlitzenCore::CE_WORLD_SYSTEM_NAME);
        }

        if (!pResources->m_textureManager.AddMaterial(0, 0, 0, 0, BlitzenCore::Ce_DefaultMaterialName))
        {
            BLIT_ERROR("Rendering resources failed");
            return false;
        }

        pResources->m_meshContext.m_triangles.ALLOC();
        pResources->m_meshContext.m_clusters.ALLOC();

#if defined(CUSTOM_FILE_TEST) && !defined(MOVING_RESIDENT_TEST) && !defined(DEFAULT_GLTF_SCENE_TEST) && !defined(LOAD_CMD_ARG_GLTF_FILEPATHS) && !defined(RENDERER_STRESS_TEST)
        // Skip hardcoded load
#else
        uint32_t bunnyMeshId{ LoadMeshFromObj(pResources->m_meshContext, "Assets/Meshes/bunny.obj", BlitzenCore::Ce_DefaultMeshName) };
        if (bunnyMeshId == BlitzenCore::Ce_MaxMeshCount)
        {
            BLIT_ERROR("Failed to load default bunny mesh");
            return false;
        }

        uint32_t kittenMeshId{ LoadMeshFromObj(pResources->m_meshContext, "Assets/Meshes/kitten.obj", BlitzenCore::Ce_DefaultKittenMeshName) };
        if (kittenMeshId == BlitzenCore::Ce_MaxMeshCount)
        {
            BLIT_ERROR("Failed to load default kitten mesh");
            return false;
        }

        uint32_t dragonMeshId{ LoadMeshFromObj(pResources->m_meshContext, "Assets/Meshes/dragon.obj", BlitzenCore::Ce_DefaultDragonMeshName) };
        if (dragonMeshId == BlitzenCore::Ce_MaxMeshCount)
        {
            BLIT_ERROR("Failed to load default dragon mesh");
            return false;
        }

        uint32_t humanMeshId{ LoadMeshFromObj(pResources->m_meshContext, "Assets/Meshes/FinalBaseMesh.obj", BlitzenCore::Ce_DefaultHumanMeshname) };
        if (humanMeshId == BlitzenCore::Ce_MaxMeshCount)
        {
            BLIT_ERROR("Failed to load default human mesh");
            return false;
        }
#endif

        BlitzenEngine::InitializeMeshResourcesPointer_STATIC_ACCESS(&pResources->m_meshContext);

        // Success
        return true;
    }

    void INITIALIZE_WORLD_POINTER(WORLD_blit* ptr)
    {
        BLIT_ASSERT_MESSAGE(p_BLITZEN_WORLD == nullptr, "Tried to reinitialize WORLD pointer");
        p_BLITZEN_WORLD = ptr;
    }

    void RegisterFrameEvent(BlitzenEngine::WORLD_VARIABLE worldVariable, BlitzenCore::FrameEventPfn function)
    {
        p_BLITZEN_WORLD->m_frameEvents.RegisterFrameEvent(worldVariable, function);
    }
}