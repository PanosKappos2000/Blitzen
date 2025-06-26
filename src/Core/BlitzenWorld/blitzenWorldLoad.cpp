#include "blitzenWorldPrivate.h"
#include "BlitCL/blitclDebug.h"
#include "Renderer/Scene/blitScene.h"
#include "Renderer/Scene/gltfScene.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenWorld
{
    void LoadingLoop(int argc, char** argv, BLITZEN_SYSTEM_CONTEXT& context, BlitzenEngine::DrawContext& drawContext)
    {
        BLIT_ASSERT(RenderingResourcesInit(context.pRenderingResources, context.pWORLD->P_RENDERER.Data()));

        BlitzenEngine::InitializeWorldResidentsPointer_STATIC_ACCESS(&context.pWORLD->m_residents);
        BlitzenEngine::InitializeWorldVariableContextPtr_STATIC_ACCESS(&context.pWORLD->m_worldVariables);
        BlitzenEngine::InitializeComponentSystemPointer_STATIC_ACCESS(context.pComponents);
        
#if defined(MOVING_RESIDENT_TEST)
        BlitzenEngine::WV_CONTEXT wvContext{};
        wvContext.m_wvTypeCount = 1;
        BlitzenEngine::WVDESC wvDescs[1]{};
        wvDescs[0].m_instanceCount = 5'000;
		wvDescs[0].m_maxInstances = 5'000;
        wvDescs[0].m_typeSize = sizeof(BlitzenEngine::WVRotatingKitten);
        wvDescs[0].m_wv_type.id = 0;
        context.pWORLD->m_worldVariables.AddClientWorldVariableDescriptions(wvContext, wvDescs, 1);
#endif

        while (true)
        {
            if (context.BLITZEN_ENGINE.m_state != BlitzenCore::EngineState::LOADING)
            {
                continue;
            }

#if defined(RENDERER_STRESS_TEST)

            BlitzenEngine::SCENE_CREATE_CONTEXT stressTestCtx{};
            stressTestCtx.m_name = "Renderer Stress Test Scene";
            stressTestCtx.m_type = BlitzenEngine::SceneType::RendererStressTest;
            stressTestCtx.pRenderer = context.pWORLD->P_RENDERER.Data();
            stressTestCtx.pResidents = &context.pWORLD->m_residents;
            stressTestCtx.pResources = context.pRenderingResources;

            auto stressTestSceneRes{ BlitzenEngine::CreateScene(&context.pWORLD->m_scenes[context.pWORLD->m_sceneCount++], stressTestCtx)};

            BLIT_ASSERT(!BlitzenCore::BLIT_CHECK_FATAL((int64_t)stressTestSceneRes));
            if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)stressTestSceneRes))
            {
                BLIT_ERROR("%s: Failed to load renderer stress test scene. The engine will continue but there will be unexpected behaviour.", BlitzenCore::CE_BLITZEN_LOADING_LOOP_NAME);
            }
#endif

#if defined(MOVING_RESIDENT_TEST)

            BlitzenEngine::SCENE_CREATE_CONTEXT movingResidentSceneCtx{};
            movingResidentSceneCtx.m_name = "Moving Residents Test Scene";
            movingResidentSceneCtx.m_type = BlitzenEngine::SceneType::MovingResidentTest;
            movingResidentSceneCtx.pRenderer = context.pWORLD->P_RENDERER.Data();
            movingResidentSceneCtx.pResidents = &context.pWORLD->m_residents;
            movingResidentSceneCtx.pResources = context.pRenderingResources;

            auto movingResidentsSceneRes{ BlitzenEngine::CreateScene(&context.pWORLD->m_scenes[context.pWORLD->m_sceneCount++], movingResidentSceneCtx)};

            BLIT_ASSERT(!BlitzenCore::BLIT_CHECK_FATAL((int64_t)movingResidentsSceneRes));
            if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)movingResidentsSceneRes))
            {
                BLIT_ERROR("%s: Failed to load moving residents test scene. The engine will continue but there will be unexpected behaviour", BlitzenCore::CE_BLITZEN_LOADING_LOOP_NAME);
            }
                
#endif

#if defined(DEFAULT_GLTF_SCENE_TEST)

            BlitzenEngine::SCENE_CREATE_CONTEXT defaultGltfSceneCtx{};
            defaultGltfSceneCtx.m_name = BlitzenCore::Ce_PrimaryGltfTestScene;
            defaultGltfSceneCtx.m_type = BlitzenEngine::SceneType::GltfSceneTest;
            defaultGltfSceneCtx.pRenderer = context.pWORLD->P_RENDERER.Data();
            defaultGltfSceneCtx.pResidents = &context.pWORLD->m_residents;
            defaultGltfSceneCtx.pResources = context.pRenderingResources;

            auto defaultGltfRes{ BlitzenEngine::CreateScene(&context.pWORLD->m_scenes[context.pWORLD->m_sceneCount++], defaultGltfSceneCtx) };

            BLIT_ASSERT(!BlitzenCore::BLIT_CHECK_FATAL((int64_t)defaultGltfRes));
            if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)defaultGltfRes))
            {
                BLIT_ERROR("Failed to load default gltf scene. The engine will continue but there will be unexpected behaviour");
            }

#endif

#if defined(LOAD_CMD_ARG_GLTF_FILEPATHS)

            if (argc > 1)
            {
                BlitzenEngine::SCENE_CREATE_CONTEXT cmdArgGltfContext{};
                cmdArgGltfContext.m_name = argv[1];
                cmdArgGltfContext.m_type = BlitzenEngine::SceneType::GltfSceneTest;
                cmdArgGltfContext.pRenderer = context.pWORLD->P_RENDERER.Data();
                cmdArgGltfContext.pResidents = &context.pWORLD->m_residents;
                cmdArgGltfContext.pResources = context.pRenderingResources;

                auto cmdArgGltfRes{ BlitzenEngine::CreateScene(&context.pWORLD->m_scenes[context.pWORLD->m_sceneCount++], cmdArgGltfContext) };

                BLIT_ASSERT(!BlitzenCore::BLIT_CHECK_FATAL((int64_t)cmdArgGltfRes));
                if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)cmdArgGltfRes))
                {
                    BLIT_ERROR("Failed to load default gltf scene. The engine will continue but there will be unexpected behaviour");
                }
            }
#endif

            if (!BlitzenEngine::UploadResourcesToGPU(context.pWORLD->P_RENDERER.Data(), drawContext))
            {
                BLIT_FATAL("Renderer failed to setup, Blitzen shutting down");

                context.BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::SHUTDOWN;
                return;
            }

            context.BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::SETUP_AFTER_LOAD;

            BlitzenPlatform::PutMouseInGameState(context.pPlatform);
        }
    }
}