#pragma once

#include "Core/blitEvents.h"

#define BLITZEN_VERSION                 "Blitzen Engine 0"

// Window macros
#define BLITZEN_WINDOW_STARTING_X       100
#define BLITZEN_WINDOW_STARTING_Y       100
#define BLITZEN_WINDOW_WIDTH            1280
#define BLITZEN_WINDOW_HEIGHT           768
#define BLITZEN_ZNEAR                   0.01f
#define BLITZEN_FOV                     BlitML::Radians(70.f)
#define BLITZEN_DRAW_DISTANCE           600.f

namespace BlitzenEngine
{
    class Engine
    {
    public:

        Engine(); 

        void Run(uint32_t argc, char* argv[]);

        void Shutdown();

        inline void RequestShutdown() { isRunning = 0; }

        inline static Engine* GetEngineInstancePointer() { return s_pEngine; }

        void UpdateWindowSize(uint32_t newWidth, uint32_t newHeight);

        // Returns delta time, crucial for functions that move objects
        inline double GetDeltaTime() { return m_deltaTime; }

    private:

        // Makes sure that the engine is only created once and gives access subparts of the engine through static getter
        static Engine* s_pEngine;

        uint8_t isRunning = 0;
        uint8_t isSupended = 0;

        // Clock / DeltaTime values (will be calulated using platform specific system calls at runtime)
        double m_clockStartTime = 0;
        double m_clockElapsedTime = 0;
        double m_deltaTime;
    };

    void RegisterDefaultEvents();

    // Will be registered to the event system on initalization
    uint8_t OnEvent(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);
    uint8_t OnKeyPress(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);
    uint8_t OnResize(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);
    uint8_t OnMouseMove(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);
}
