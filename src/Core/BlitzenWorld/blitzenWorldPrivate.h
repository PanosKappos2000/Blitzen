#pragma once
#include "Core/Dasher/Interface/dasherInterface.h"
#include "Renderer/WORLD/blitzenWorld.h"
#include "Renderer/Entities/Interface/blitComponents.h"
#include "WorldVariables/wvData.h"
#include "Core/Events/blitKeys.h"
#include "Core/Events/blitController.h"
#include "Core/Events/blitEditorEvents.h"
#include "Core/Events/blitFrameEvents.h"

namespace BlitzenWorld
{
    // Standard events
    using EventCallback = BlitCL::Pfn<uint8_t, BLIT_STRAIGHTHANDLE, BlitzenCore::BlitEventType>;

    // Editor events take full context and return controller id
    using EditorCallback = BlitCL::Pfn<uint32_t, BLIT_STRAIGHTHANDLE>;

    enum class ControllerState : uint8_t
    {
        Editor = 0x0,
        Game = 0x1,
        Max = UINT8_MAX
    };

#if defined(DASHER_JOIN)
    constexpr ControllerState CE_INITIAL_CONTROLLER_STATE = ControllerState::Editor;
#else
    constexpr ControllerState CE_INITIAL_CONTROLLER_STATE = ControllerState::Game;
#endif

    constexpr uint32_t CE_INITIAL_CONTROLLER_ID = 0;
    constexpr uint32_t CE_STARTING_CONTROLLER_COUNT = 2;

    struct BLITZEN_SYSTEM_CONTEXT
    {
        BlitzenCore::Engine BLITZEN_ENGINE;

		// WINDOW
        uint32_t WIDTH{ BlitzenCore::Ce_InitialWindowWidth };
        uint32_t HEIGHT{ BlitzenCore::Ce_InitialWindowHeight };
        
        // SYSTEMS
        BlitzenEngine::RenderingResources* pRenderingResources{ nullptr };
        BlitzenEngine::ComponentSystem* pComponents{ nullptr };
        BlitzenPlatform::PlatformContext* pPlatform{ nullptr };
        BlitzenCore::Dasher* pDasher{ nullptr };
        BlitzenCore::WorldTimeManager* pClock;

        // WORLD
        WORLD_blit* pWORLD{ nullptr };

        // EVENTS
        EventCallback m_eventCallbacks[uint32_t(BlitzenCore::BlitEventType::MaxTypes)]{};
        BlitzenCore::Controller m_controllers[BlitzenCore::Ce_MaxControllerCount];
        uint32_t m_activeControllerIDX{ CE_INITIAL_CONTROLLER_ID };
        uint32_t m_controllerCount{ CE_STARTING_CONTROLLER_COUNT };
        EditorCallback m_editorButtonCallbacks[BlitzenCore::Ce_EditorButtonEventTypeCount]{ [](BLIT_STRAIGHTHANDLE)->uint32_t {return CE_INITIAL_CONTROLLER_ID; } };
        BlitzenCore::FAT_BOOL m_currentKeyboard[BlitzenCore::Ce_KeyCallbackCount];
        BlitzenCore::FAT_BOOL m_previousKeyboard[BlitzenCore::Ce_KeyCallbackCount];
        BlitzenCore::FAT_BOOL m_mouseButtonFlags[uint8_t(BlitzenCore::MouseButton::MaxButtons)];
        ControllerState m_controllerState{ CE_INITIAL_CONTROLLER_STATE };
    };

    void LoadingLoop(int argc, char** argv, BLITZEN_SYSTEM_CONTEXT& context, BlitzenEngine::DrawContext& drawContext);

    void BMPR_DRIVE(BLITZEN_SYSTEM_CONTEXT& context);

    void WV_DRIVE(BLITZEN_SYSTEM_CONTEXT& context);

    void WorldLoop(BLITZEN_SYSTEM_CONTEXT& context);

    void S_WORLD_UPDATE_RESIDENT_MOVED(uint32_t id);
}