#pragma once
#include "BlitCL/blitPfn.h"
#include "Renderer/Scene/blitScene.h"
#include "Core/Events/blitFrameEvents.h"
#include "Renderer/View/blitCamera.h"

namespace BlitzenWorld
{
    constexpr uint32_t CE_MAIN_CHARACTER_ID = 0;

    struct WorldDebugData
    {
        bool drawCollidersFlag = false;
        BlitML::float3* collisionGridVertices = nullptr;
    };

    // WORLD variable. Represent the idea of world interaction
    class BLITZEN_WORLD
    {
    public:

        void DispatchFrameEvents(float deltaTime);

        // Scene resources
        BlitzenEngine::SceneContext m_scenes[10]{};
        uint32_t m_sceneCount{ 0 };

        BlitzenEngine::Renderer BMPR{};
        BlitzenEngine::DrawContext m_drawContext;

        // World residents
        BlitzenEngine::WORLD_RESIDENTS mResidents{};
        BlitzenEngine::CollisionGrid mCollisionGrid{};
        BlitzenCore::FrameEventManager m_frameEvents;
        BlitzenEngine::Resident m_mainCharacter = 0;
        BlitzenEngine::Camera m_cameras[BlitzenCore::CE_STARTING_CONTROLLER_COUNT]{};
        uint32_t m_activeCameraIDX = BlitzenCore::CE_ENGINE_CONTROLLER_ID;
        BlitzenEngine::CollisionWorkConstant MBmprCollisionWorkConstant;

        float deltaTime{0.f};

#if !defined (NDEBUG)
        WorldDebugData mDbgData;
#endif

        inline BLITZEN_WORLD(BlitzenEngine::MeshResources& meshes, BlitzenEngine::TextureManager& textureManager, BlitzenPlatform::PlatformContext* pPlatform)
            :m_drawContext{ m_cameras[1], meshes, textureManager, pPlatform}
        {

        }

        ~BLITZEN_WORLD();
    };

	// Initializes some basic rendering resources, for the renderer to work out of the box.
    bool RenderingResourcesInit(BlitzenEngine::RenderingResources* pResources, BlitzenEngine::RendererPtrType pRenderer, BlitzenEngine::RenderingLoadingContextMesh& loadingContextMesh);

    // Copies vertex data and their indices for a single mesh to the staging buffer.
    // It resets the count of vertices and indices for the next mesh, but it keeps a map count.
    bool CopyMeshResourcesToStagingBuffer(BlitzenEngine::MeshResources* pResources, BlitzenEngine::RenderingLoadingContextMesh& loadingContextMesh);

    void LOAD_RESOURCES_MK_BLIT_MINUS(BLITZEN_WORLD* pWORLD, BlitzenEngine::RenderingResources* pRenderingResources, BlitzenEngine::RenderingLoadingContextMesh& loadingContextMesh,
        int argc, char** argv);

    void RegisterFrameEvent(BlitzenEngine::WORLD_VARIABLE worldVariable, BlitzenCore::FrameEventPfn function);

    void DispatchCollisionSystems(BLITZEN_WORLD* pWORLD);

    void ResolveCollisionEvents(BlitzenEngine::ColliderContainer& colliders);

    void INITIALIZE_WORLD_POINTER(BLITZEN_WORLD* ptr);

    void DispatchBumper(BlitzenWorld::BLITZEN_WORLD* WORLD, uint32_t terrainCount);

    void SNAP_MAIN();
}