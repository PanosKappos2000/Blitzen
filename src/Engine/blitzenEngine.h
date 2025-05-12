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
    constexpr uint32_t ce_initialWindowHeight = 720;
    constexpr float Ce_DefaultWindowBackgroundColor[4] = { 0.f, 0.2f, 0.4f, 1.f };


    // Camera initial settings
    constexpr float ce_initialCameraX = 20.f;
    constexpr float ce_initialCameraY = 70.f;
    constexpr float ce_initialCameraZ = 0.f;
    constexpr float ce_znear = 0.1f;
    constexpr float ce_initialFOV = 70.f;
    constexpr float ce_initialDrawDistance = 650.f;


    // Other camera constants
    constexpr uint8_t MaxCameraCount = 1;
    constexpr uint8_t MainCameraId = 0;


    // Renderer settings
    constexpr uint32_t DDSCAPS2_CUBEMAP = 0x200;
    constexpr uint32_t DDSCAPS2_VOLUME = 0x200000;
    constexpr uint32_t DDS_DIMENSION_TEXTURE2D = 3;

    constexpr uint32_t ce_maxTextureCount = 5'000;
    constexpr uint32_t ce_maxMaterialCount = 10'000;

    constexpr uint8_t Ce_MaxLodCountPerSurface = 8;
    constexpr uint32_t Ce_MaxInstanceCountPerLOD = 100'000;
    constexpr uint32_t Ce_MaxInstanceCountPerCluster = 100'000;

    constexpr uint32_t ce_maxMeshCount = 1'000'000; 
	constexpr const char* ce_defaultMeshName = "bunny";

    constexpr uint32_t ce_maxRenderObjects = 5'000'000;
    constexpr uint32_t ce_maxONPC_Objects = 100;
    constexpr uint32_t Ce_MaxDynamicObjectCount = 1'000;

    #ifdef BLITZEN_CLUSTER_CULLING
        constexpr uint8_t Ce_BuildClusters = 1;
    #else
        constexpr uint8_t Ce_BuildClusters = 0;
    #endif

    #ifdef BLITZEN_DRAW_INSTANCED_CULLING
        constexpr uint8_t Ce_InstanceCulling = 1;
    #else
        constexpr uint8_t Ce_InstanceCulling = 0;
    #endif

    #if defined(_WIN32) && !defined(BLIT_VK_FORCE) && !defined(BLIT_GL_LEGACY_OVERRIDE)
        constexpr bool Ce_HLSL = 1;// Row major?
    #else
        constexpr bool Ce_HLSL = 0;
    #endif
    

    constexpr const char* ce_blitzenVersion = "Blitzen Engine";
    constexpr uint32_t ce_blitzenMajor = 0;
    constexpr const char* Ce_HostedApp = "Blitzen Game";
    constexpr uint32_t Ce_HostedAppVersion = 1;

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

        bool m_bRunning = false;
        bool m_bSuspended = false;
    };
}
