#pragma once

#include "Core/blitAssert.h"
#include "Platform/platform.h"
#include "Core/blitzenContainerLibrary.h"
#include "Core/blitEvents.h"
#include "BlitzenVulkan/vulkanRenderer.h"

#define BLITZEN_VERSION                 "Blitzen Engine 1.0.C"

#define BLITZEN_VULKAN                  1

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

    struct Clock
    {
        double startTime = 0;
        double elapsedTime = 0;
    };

    struct EngineSystems
    {
        uint8_t eventSystem = 0;
        // The event state is owned by the engine so that the dynamic array inside it, get freed before memory management is shutdown
        BlitzenCore::EventSystemState eventState;

        uint8_t inputSystem = 0;

        uint8_t logSystem = 0;

        #if BLITZEN_VULKAN
            BlitzenVulkan::VulkanRenderer vulkanRenderer;
            uint8_t vulkan = 0;
        #endif
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

        inline EngineSystems& GetEngineSystems() { return m_systems; }

    private:

        void StartClock();
        void StopClock();

        /* ---------------------------------------------------
            Dedicated function for the renderers. 
            If Blitzen gets more than one renderer, 
            this will decide which ones will be initialized 
            and which one will draw the frame.
        -------------------------------------------------------*/
        uint8_t RendererInit();

    private:

        // Makes sure that the engine is only created once and gives access subparts of the engine through static getter
        static Engine* pEngineInstance;

        uint8_t isRunning = 0;
        uint8_t isSupended = 0;

        // Used to create the window and other platform specific part of the engine
        BlitzenPlatform::PlatformState m_platformState;
        PlatformStateData m_platformData;

        Clock m_clock;

        void* pRenderer = nullptr;

        EngineSystems m_systems;
    };

    // Will be registered to the event system on initalization
    uint8_t OnEvent(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);
    uint8_t OnKeyPress(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);

}
