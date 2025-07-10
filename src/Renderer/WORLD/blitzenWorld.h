#pragma once
#include "BlitCL/blitPfn.h"
#include "Renderer/Scene/blitScene.h"
#include "Core/Events/blitFrameEvents.h"
#include "Renderer/View/blitCamera.h"

namespace BlitzenWorld
{
    constexpr uint32_t CE_MAIN_CHARACTER_ID = 0;

    // WORLD variable. Represent the idea of world interaction
    class WORLD_blit
    {
    public:

        void DispatchFrameEvents(float deltaTime);

        // Scene resources
        BlitzenEngine::SceneContext m_scenes[10]{};
        uint32_t m_sceneCount{ 0 };

        BlitzenEngine::Renderer P_RENDERER{};
        BlitzenEngine::DrawContext m_drawContext;

        // World residents
        BlitzenEngine::WORLD_RESIDENTS m_residents{};
        BlitzenEngine::CollisionGrid m_collisionGrid{};
        BlitzenCore::FrameEventManager m_frameEvents;
        BlitzenEngine::Resident m_mainCharacter = 0;
        BlitzenEngine::Camera m_cameras[BlitzenCore::CE_STARTING_CONTROLLER_COUNT]{};
        uint32_t m_activeCameraIDX = BlitzenCore::CE_ENGINE_CONTROLLER_ID;

        float deltaTime{0.f};

        inline WORLD_blit(BlitzenEngine::MeshResources& meshes, BlitzenEngine::TextureManager& textureManager, BlitzenPlatform::PlatformContext* pPlatform)
            :m_drawContext{ m_cameras[1], meshes, textureManager, pPlatform}
        {

        }
    };

	// Initializes some basic rendering resources, for the renderer to work out of the box.
    bool RenderingResourcesInit(BlitzenEngine::RenderingResources* pResources, BlitzenEngine::RendererPtrType pRenderer, BlitzenEngine::RenderingLoadingContextMesh& loadingContextMesh);

    // Copies vertex data and their indices for a single mesh to the staging buffer.
    // It resets the count of vertices and indices for the next mesh, but it keeps a map count.
    bool CopyMeshResourcesToStagingBuffer(BlitzenEngine::MeshResources* pResources, BlitzenEngine::RenderingLoadingContextMesh& loadingContextMesh);

    void LOAD_RESOURCES_MK_BLIT_MINUS(WORLD_blit* pWORLD, BlitzenEngine::RenderingResources* pRenderingResources, BlitzenEngine::RenderingLoadingContextMesh& loadingContextMesh, 
        int argc, char** argv);

    void RegisterFrameEvent(BlitzenEngine::WORLD_VARIABLE worldVariable, BlitzenCore::FrameEventPfn function);

    void INITIALIZE_WORLD_POINTER(WORLD_blit* ptr);

    void SNAP_MAIN();
}