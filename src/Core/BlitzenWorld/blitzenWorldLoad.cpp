#include "blitzenWorldPrivate.h"
#include "BlitCL/blitclDebug.h"
#include "Renderer/Scene/blitScene.h"
#include "Renderer/Scene/gltfScene.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"
#include "Core/Events/blitEvents.h"
#include "blitzenUserInterface.h"

namespace BlitzenWorld
{
    static void DRIVE_SYSTEM_REQUEST(BLITZEN_SYSTEM_CONTEXT& SYSTEM, ENGINE_SYSTEM_DRIVE_REQUEST drive);

    void LoadingLoop(int argc, char** argv, BLITZEN_SYSTEM_CONTEXT& context, BlitzenEngine::DrawContext& drawContext)
    {
        BLIT_ASSERT(RenderingResourcesInit(context.pRenderingResources, context.pWORLD->P_RENDERER.Data()));

        BlitzenPlatform::MakeWindowVisible(context.pPlatform);
        context.BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::LOADING;

        BlitzenEngine::InitializeWorldResidentsPointer_STATIC_ACCESS(&context.pWORLD->m_residents);
        //BlitzenEngine::InitializeWorldVariableContextPtr_STATIC_ACCESS(&context.pWORLD->m_worldVariables);
        BlitzenEngine::InitializeComponentSystemPointer_STATIC_ACCESS(context.pComponents);
        INITIALIZE_WORLD_POINTER(context.pWORLD);

        JobDriverQueue driver;
        StartJobSystem(&driver);

        while (true)
        {
            if (context.BLITZEN_ENGINE.m_state != BlitzenCore::EngineState::LOADING)
            {
                continue;
            }

            BlitzenEngine::SCENE_CREATE_CONTEXT sceneCtx{};
            sceneCtx.pRenderer = context.pWORLD->P_RENDERER.Data();
            sceneCtx.pResidents = &context.pWORLD->m_residents;
            sceneCtx.pResources = context.pRenderingResources;

            BlitCL::DynamicArray<BlitzenEngine::SceneContext> scenes{};

#if defined(CUSTOM_FILE_TEST) && !defined(MOVING_RESIDENT_TEST) && !defined(DEFAULT_GLTF_SCENE_TEST) && !defined(LOAD_CMD_ARG_GLTF_FILEPATHS) && !defined(RENDERER_STRESS_TEST)

            BlitzenEngine::SceneContext rpf{};
            rpf.m_type = BlitzenEngine::SceneType::CustomFileTest;
            scenes.PushBack(rpf);

#endif

#if defined(RENDERER_STRESS_TEST)

            BlitzenEngine::SceneContext stress{};
            stress.m_type = BlitzenEngine::SceneType::RendererStressTest;
            scenes.PushBack(stress);

#endif

            BlitzenEngine::SceneContext moving{};
            moving.m_type = BlitzenEngine::SceneType::MovingResidentTest;
            scenes.PushBack(moving);

#if defined(DEFAULT_GLTF_SCENE_TEST)

            BlitzenEngine::SceneContext defaultGltf{};
            defaultGltf.m_name = BlitzenCore::Ce_PrimaryGltfTestScene;
            defaultGltf.m_type = BlitzenEngine::SceneType::GltfSceneTest;
            scenes.PushBack(defaultGltf);

#endif

#if defined(LOAD_CMD_ARG_GLTF_FILEPATHS)

            if (argc > 1)
            {
                BlitzenEngine::SceneContext defaultGltf{};
                defaultGltf.m_name = argv[1];
                defaultGltf.m_type = BlitzenEngine::SceneType::GltfSceneTest;
                scenes.PushBack(defaultGltf);
            }
#endif

#if defined(COLLISION_TEST)

            BlitzenEngine::SceneContext collisionTst{};
            collisionTst.m_type = BlitzenEngine::SceneType::SmallSceneForCollision;
            scenes.PushBack(collisionTst);

#endif
            sceneCtx.m_sceneArr = scenes.Data();
            sceneCtx.m_sceneCount = (uint32_t)scenes.GetSize();

            auto sceneRes{ BlitzenEngine::CreateScene(sceneCtx) };

            BLIT_ASSERT(!BlitzenCore::BLIT_CHECK_FATAL((int64_t)sceneRes));
            if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)sceneRes))
            {
                BLIT_ERROR("%s: Error while loading scenes. Received error message: %s", BlitzenCore::CE_BLITZEN_LOADING_LOOP_NAME, BlitzenEngine::GET_SCENE_CREATE_RES_STRING(sceneRes));
                BLIT_ASSERT(false);
            }

            context.pWORLD->m_collisionGrid.DefineGrid(0);
            context.pWORLD->m_collisionGrid.CreateCells();
            context.pWORLD->m_collisionGrid.PlaceStatics(&context.pWORLD->m_residents.m_renders.m_renders[BLIT_OPAQUE_STATIC_RENDER_OFFSET], context.pWORLD->m_residents.m_transforms.m_staticTransformCount,
                context.pWORLD->m_residents.m_transforms.m_transforms);

            // Testing, this should be done another way.
            context.m_activeControllerIDX = 1;
            context.pWORLD->m_activeCameraIDX = 1;
            context.m_controllerState = ControllerState::Game;
            BlitzenWorld::SetupCameraAttachment(context.pWORLD->m_mainCharacter, BlitML::float3(0.f, 20.f, -30.f), BlitzenEngine::CAMERA_FREE_ROTATION_SETTING::ALWAYS);

#if defined(CUSTOM_FILE_TEST) && !defined(MOVING_RESIDENT_TEST) && !defined(DEFAULT_GLTF_SCENE_TEST) && !defined(LOAD_CMD_ARG_GLTF_FILEPATHS) && !defined(RENDERER_STRESS_TEST)

#else

            if (!BlitzenEngine::UploadResourcesToGPU(context.pWORLD->P_RENDERER.Data(), drawContext))
            {
                BLIT_FATAL("Renderer failed to setup, Blitzen shutting down");

                context.BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::SHUTDOWN;
                return;
            }

#endif
            ENGINE_SYSTEM_DRIVE_REQUEST systemRequest{ ENGINE_SYSTEM_DRIVE_REQUEST::PLACEHOLDER };
            while (systemRequest != ENGINE_SYSTEM_DRIVE_REQUEST::NO_JOBS)
            {
                systemRequest = driver.Dispatch();
            }
            context.BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::SETUP_AFTER_LOAD;

            // Useless, but keeping it here to remember to do something with it
            BlitzenPlatform::PutMouseInGameState(context.pPlatform);

            context.pRenderingResources->m_meshContext.m_triangles.CLEAN();
            context.pRenderingResources->m_meshContext.m_clusters.CLEAN();
        }
    }

    static void DRIVE_SYSTEM_REQUEST(BLITZEN_SYSTEM_CONTEXT& SYSTEM, ENGINE_SYSTEM_DRIVE_REQUEST drive)
    {
        //switch (drive)
        //{
        //case ENGINE_SYSTEM_DRIVE_REQUEST::EVENT_REGISTER:
        //{
        //    BlitzenCore::RegisterEvent(&SYSTEM, )
        //}
        //}
    }
}