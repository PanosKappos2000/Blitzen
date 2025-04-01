#pragma once

#include <stdio.h>
#include <cstdint>
#include <cstddef>

//#define LAMBDA_GAME_OBJECT_TEST

namespace BlitzenEngine
{
    // Window constants
    constexpr uint32_t ce_windowStartingX = 100;
    constexpr uint32_t ce_windowStartingY = 100;
    constexpr uint32_t ce_initialWindowWidth = 1280;
    constexpr uint32_t ce_initialWindowHeight = 768;

    // Camera values
    constexpr float ce_initialCameraX = 20.f;
    constexpr float ce_initialCameraY = 70.f;
    constexpr float ce_initialCameraZ = 0.f;
    constexpr float ce_znear = 0.1f;
    constexpr float ce_initialFOV = 70.f;
    constexpr float ce_initialDrawDistance = 650.f;

    constexpr const char* ce_blitzenVersion = "Blitzen Engine 0";
    constexpr uint32_t ce_blitzenMajor = 0;

    class Engine
    {
    public:

        Engine();
        
        inline static void BeginShutdown() { s_pEngine = nullptr; }

        ~Engine();

        // Suspending the engine, means that most systems (like rendering) are inactive. 
        // But it is still running on the background
        inline void Suspend() { m_bSuspended = true; }
        inline void ReActivate() { m_bSuspended = false; }
        inline bool IsActive() const { return !m_bSuspended; }

        // If bRunning is false, the engine shuts down and the application quits.
        inline void BootUp() { m_bRunning = true; }
        inline void Shutdown() { m_bRunning = false; }
        inline bool IsRunning() const { return m_bRunning; }

        inline static Engine* GetEngineInstancePointer() { return s_pEngine; }

    private:

        // Leaky singleton
        static Engine* s_pEngine;

        bool m_bRunning = 0;
        bool m_bSuspended = 0;
    };
}
