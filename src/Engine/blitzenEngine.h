#pragma once

#include <stdio.h>
#include <cstdint>
#include <cstddef>

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
    constexpr float ce_initialDrawDistance = 600.f;

    constexpr char* ce_blitzenVersion = "Blitzen Engine 0";
    constexpr uint32_t ce_blitzenMajor = 0;

    // Honestly, this class does not need to exist, the only important thing it has is the Run function.
    // But it started off as the most important thing in the codebase and I don't really want to erase it
    class Engine
    {
    public:

        Engine(); 

        void Run(uint32_t argc, char* argv[]);

        inline void Suspend() { bSuspended = 1; }

        inline void ReActivate() { bSuspended = 0; }

        inline void Shutdown() { bRunning = 0; }

        inline static Engine* GetEngineInstancePointer() { return s_pEngine; }

        // Returns delta time, crucial for functions that move objects
        inline double GetDeltaTime() { return m_deltaTime; }

    private:

        // Makes sure that the engine is only created once and gives access subparts of the engine through static getter
        static Engine* s_pEngine;

        uint8_t bRunning = 0;
        uint8_t bSuspended = 0;

        // Clock / DeltaTime values (will be calulated using platform specific system calls at runtime)
        double m_clockStartTime = 0;
        double m_clockElapsedTime = 0;
        double m_deltaTime;
    };

    void RegisterDefaultEvents();
}
