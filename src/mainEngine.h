#pragma once

#include "Core/blitAssert.h"
#include "Platform/platform.h"
#include "Core/blitzenContainerLibrary.h"
#include "Core/blitEvents.h"

#define BLITZEN_VERSION                 "Blitzen Engine 1.0.C"

#define BLITZEN_WINDOW_STARTING_X       100
#define BLITZEN_WINDOW_STARTING_Y       100
#define BLITZEN_WINDOW_WIDTH            1280
#define BLITZEN_WINDOW_HEIGHT           720

namespace BlitzenEngine
{
    struct PlatformStateData
    {
        uint32_t windowWidth = BLITZEN_WINDOW_WIDTH;
        uint32_t windowHeight = BLITZEN_WINDOW_HEIGHT;
    };

    struct EngineSystems
    {
        uint8_t eventSystem = 0;
        uint8_t inputSystem = 0;
        uint8_t logSystem = 0;
    };

    class Engine
    {
    public:
        Engine();

        void Run();

        // I might want to explicitly shutdown the engine, as there are some systems that are initalized outside of it
        ~Engine();
        inline void RequestShutdown() { isRunning = 0; }

        inline static Engine* GetEngineInstancePointer() { return pEngineInstance; }

        inline const EngineSystems& GetEngineSystems() { return m_systems; }

    private:

    private:

        // Makes sure that the engine is only created once and gives access subparts of the engine through static getter
        static Engine* pEngineInstance;

        uint8_t isRunning = 0;
        uint8_t isSupended = 0;

        // Used to create the window and other platform specific part of the engine
        BlitzenPlatform::PlatformState m_platformState;
        PlatformStateData m_platformData;

        EngineSystems m_systems;
    };

    // Will be registered to the event system on initalization
    uint8_t OnEvent(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);
    uint8_t OnKeyPress(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);

}
