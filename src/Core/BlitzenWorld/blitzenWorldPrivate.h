#pragma once
#include "Core/Dasher/Interface/dasherInterface.h"
#include "Renderer/WORLD/blitzenWorld.h"
#include "Core/Events/blitKeys.h"
#include "Core/Events/blitController.h"
#include "Core/Events/blitEditorEvents.h"
#include "blitzenSystemDispatcher.h"
#include "Audio/blitAudio.h"

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

    struct BLITZEN_SYSTEM_CONTEXT
    {
        BlitzenCore::Engine BLITZEN_ENGINE;

		// WINDOW
        uint32_t WIDTH{ BlitzenCore::Ce_InitialWindowWidth };
        uint32_t HEIGHT{ BlitzenCore::Ce_InitialWindowHeight };
        
        // SYSTEMS
        BlitzenEngine::RenderingResources* pRenderingResources{ nullptr };
        BlitzenPlatform::PlatformContext* pPlatform{ nullptr };
        BlitzenCore::Dasher* pDasher{ nullptr };
        BlitzenEngine::DasherUI* pDASHER{ nullptr };
        BlitzenCore::WorldTimeManager* pClock;
        BlitzenEngine::AudioEngine* pJingle;

        // WORLD
        BLITZEN_WORLD* pWORLD{ nullptr };

        // EVENTS
        EventCallback m_eventCallbacks[uint32_t(BlitzenCore::BlitEventType::MaxTypes)]{};
        int32_t mMouseDeltaXAxis{ 0 };
        int32_t mMouseDeltaYAxis{ 0 };
        BlitzenCore::Controller* m_controllers;
        uint32_t m_activeControllerIDX{ BlitzenCore::CE_INITIAL_CONTROLLER_ID };
        uint32_t m_controllerCount{ BlitzenCore::CE_STARTING_CONTROLLER_COUNT };
        EditorCallback m_editorButtonCallbacks[BlitzenCore::Ce_EditorButtonEventTypeCount]{ [](BLIT_STRAIGHTHANDLE)->uint32_t {return BlitzenCore::CE_INITIAL_CONTROLLER_ID; } };
        BlitzenCore::FAT_BOOL m_mouseButtonFlags[uint8_t(BlitzenCore::MouseButton::MaxButtons)];
        ControllerState m_controllerState{ ControllerState::Editor };
    };

    void LoadingLoop(int argc, char** argv, BLITZEN_SYSTEM_CONTEXT& context, BlitzenEngine::DrawContext& drawContext);

    void BlitzenSystemsInit(BLITZEN_SYSTEM_CONTEXT* pSYSTEM);

    void BMPR_DRIVE(BLITZEN_SYSTEM_CONTEXT& context);

    void WorldLoop(BLITZEN_SYSTEM_CONTEXT& context);
}