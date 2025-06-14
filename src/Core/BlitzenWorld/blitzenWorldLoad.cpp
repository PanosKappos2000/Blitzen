#include "blitzenWorldPrivate.h"
#include "BlitCL/blitclDebug.h"
#include "Renderer/Scene/blitScene.h"
#include "Renderer/Scene/gltfScene.h"

namespace BlitzenWorld
{
    void LoadingLoop(int argc, char** argv, BlitzenPrivateContext& context, BlitzenEngine::DrawContext& drawContext)
    {
        BLIT_ASSERT(BlitzenEngine::RenderingResourcesInit(context.pRenderingResources, context.pWORLD->P_RENDERER.Data()));

        while (true)
        {
            if (*context.pEngineState == BlitzenCore::EngineState::LOADING)
            {
#if defined(RENDERER_STRESS_TEST)

                BlitzenEngine::SCENE_CREATE_CONTEXT stressTestCtx{};
                stressTestCtx.m_name = "Renderer Stress Test Scene";
                stressTestCtx.m_type = BlitzenEngine::SceneType::RendererStressTest;
                stressTestCtx.pRenderer = context.pWORLD->P_RENDERER.Data();
                stressTestCtx.pResidents = context.pWORLD->P_RESIDENTS.Data();
                stressTestCtx.pResources = context.pRenderingResources;

                context.pWORLD->m_scenes.EmplaceEmtpy();
                auto stressTestSceneRes{ BlitzenEngine::CreateScene(&context.pWORLD->m_scenes.Back(), stressTestCtx) };

                BLIT_ASSERT(!BlitzenCore::BLIT_CHECK_FATAL(stressTestSceneRes));
                if (BlitzenCore::BLIT_CHECK_FAIL(stressTestSceneRes))
                {
                    BLIT_ERROR("Failed to load renderer stress test scene. The engine will continue but there will be unexpected behaviour.");
                }
#endif

#if defined(MOVING_RESIDENT_TEST)

                BlitzenEngine::SCENE_CREATE_CONTEXT movingResidentSceneCtx{};
                movingResidentSceneCtx.m_name = "Moving Residents Test Scene";
                movingResidentSceneCtx.m_type = BlitzenEngine::SceneType::MovingResidentTest;
                movingResidentSceneCtx.pRenderer = context.pWORLD->P_RENDERER.Data();
                movingResidentSceneCtx.pResidents = context.pWORLD->P_RESIDENTS.Data();
                movingResidentSceneCtx.pResources = context.pRenderingResources;

                context.pWORLD->m_scenes.EmplaceEmtpy();
                auto movingResidentsSceneRes{ BlitzenEngine::CreateScene(&context.pWORLD->m_scenes.Back(), movingResidentSceneCtx) };

                BLIT_ASSERT(!BlitzenCore::BLIT_CHECK_FATAL(movingResidentsSceneRes));
                if (BlitzenCore::BLIT_CHECK_FAIL(movingResidentsSceneRes))
                {
                    BLIT_ERROR("Failed to load moving residents test scene. The engine will continue but there will be unexpected behaviour");
                }
                
#endif

#if defined(DEFAULT_GLTF_SCENE_TEST)

                BlitzenEngine::SCENE_CREATE_CONTEXT defaultGltfSceneCtx{};
                defaultGltfSceneCtx.m_name = "Default Gltf Scene";
                defaultGltfSceneCtx.m_type = BlitzenEngine::SceneType::GltfSceneTest;
                defaultGltfSceneCtx.pRenderer = context.pWORLD->P_RENDERER.Data();
                defaultGltfSceneCtx.pResidents = context.pWORLD->P_RESIDENTS.Data();
                defaultGltfSceneCtx.pResources = context.pRenderingResources;

                context.pWORLD->m_scenes.EmplaceEmtpy();
                auto defaultGltfRes{ BlitzenEngine::CreateScene(&context.pWORLD->m_scenes.Back(), defaultGltfSceneCtx) };

                BLIT_ASSERT(!BlitzenCore::BLIT_CHECK_FATAL(defaultGltfRes));
                if (BlitzenCore::BLIT_CHECK_FAIL(defaultGltfRes))
                {
                    BLIT_ERROR("Failed to load default gltf scene. The engine will continue but there will be unexpected behaviour");
                }

#endif

#if defined(LOAD_CMD_ARG_GLTF_FILEPATHS)

                BlitzenEngine::SCENE_CREATE_CONTEXT cmdArgGltfContext{};
                cmdArgGltfContext.m_name = "CmdArg Gltf Scene";
                cmdArgGltfContext.m_type = BlitzenEngine::SceneType::GltfSceneTest;
                cmdArgGltfContext.pRenderer = context.pWORLD->P_RENDERER.Data();
                cmdArgGltfContext.pResidents = context.pWORLD->P_RESIDENTS.Data();
                cmdArgGltfContext.pResources = context.pRenderingResources;

                context.pWORLD->m_scenes.EmplaceEmtpy();
                auto cmdArgGltfRes{ BlitzenEngine::CreateScene(&context.pWORLD->m_scenes.Back(), cmdArgGltfContext) };

                BLIT_ASSERT(!BlitzenCore::BLIT_CHECK_FATAL(cmdArgGltfRes));
                if (BlitzenCore::BLIT_CHECK_FAIL(cmdArgGltfRes))
                {
                    BLIT_ERROR("Failed to load default gltf scene. The engine will continue but there will be unexpected behaviour");
                }
#endif

                if (!BlitzenEngine::UploadResourcesToGPU(context.pWORLD->P_RENDERER.Data(), drawContext))
                {
                    BLIT_FATAL("Renderer failed to setup, Blitzen shutting down");
                    
                    *context.pEngineState = BlitzenCore::EngineState::SHUTDOWN;
                    return;
                }

                *context.pEngineState = BlitzenCore::EngineState::SETUP_AFTER_LOAD;

                BlitCL::LogContainerData(context.pWORLD->m_scenes, "Scene");
            }
        }
    }
}