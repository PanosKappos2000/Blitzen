#include "blitzenWorldPrivate.h"
#include "BlitCL/blitclDebug.h"

namespace BlitzenWorld
{
    void LoadingLoop(int argc, char** argv, BlitzenPrivateContext& context, BlitzenEngine::DrawContext& drawContext)
    {
        while (true)
        {
            if (*context.pEngineState == BlitzenCore::EngineState::LOADING)
            {
                std::lock_guard<std::mutex> lock(context.m_resourceLoadingMutex);

                if (!BlitzenEngine::CreateSceneFromArguments(argc, argv, context.pRenderingResources, context.pWORLD, context.pEntityMangager))
                {
                    BLIT_FATAL("Failed to allocate resource for requested scene, Blitzen shutting down");
                    
                    *context.pEngineState = BlitzenCore::EngineState::SHUTDOWN;
                    return;
                }

                if (!context.pWORLD->P_RENDERER->SetupForRendering(drawContext))
                {
                    BLIT_FATAL("Renderer failed to setup, Blitzen shutting down");
                    
                    *context.pEngineState = BlitzenCore::EngineState::SHUTDOWN;
                    return;
                }

                *context.pEngineState = BlitzenCore::EngineState::SETUP_AFTER_LOAD;

                BlitCL::LogContainerData(context.pWORLD->m_scenes, "Scene");

                return;
            }
        }
    }
}