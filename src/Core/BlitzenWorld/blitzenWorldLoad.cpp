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
        INITIALIZE_WORLD_POINTER(context.pWORLD);
        BLIT_ASSERT(RenderingResourcesInit(context.pWORLD, context.pRenderingResources, context.pWORLD->BMPR.Data()));

        BlitzenPlatform::MakeWindowVisible(context.pPlatform);
        context.BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::LOADING;

        while (true)
        {
            if (context.BLITZEN_ENGINE.m_state != BlitzenCore::EngineState::LOADING)
            {
                continue;
            }

            BlitzenWorld::LOAD_RESOURCES_MK_BLIT_MINUS(context.pWORLD, context.pRenderingResources, argc, argv);

            // Testing, this should be done another way.
#if defined(BLIT_GAME_TEST)
            context.m_activeControllerIDX = 1;
            context.m_controllerState = ControllerState::Game;
#endif
            BLIT_DBLOG("OUT");
            
            context.BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::SETUP_AFTER_LOAD;

            // Useless, but keeping it here to remember to do something with it
            BlitzenPlatform::PutMouseInGameState(context.pPlatform);
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