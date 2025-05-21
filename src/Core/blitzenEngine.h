#pragma once
#include <stdio.h>
#include <cstdint>
#include <cstddef>

namespace BlitzenCore
{
    // Window constants
    constexpr uint32_t Ce_WindowStartingX = 100;
    constexpr uint32_t Ce_WindowStartingY = 100;
    constexpr uint32_t Ce_InitialWindowWidth = 1280;
    constexpr uint32_t Ce_InitialWindowHeight = 720;
    constexpr float Ce_DefaultWindowBackgroundColor[4] = { 0.f, 0.2f, 0.4f, 1.f };


    // Camera initial settings
    constexpr float Ce_InitialCameraX = 20.f;
    constexpr float Ce_initialCameraY = 70.f;
    constexpr float Ce_initialCameraZ = 0.f;
    constexpr float Ce_Znear = 0.1f;
    constexpr float Ce_InitialFOV = 70.f;
    constexpr float Ce_InitialDrawDistance = 650.f;


    // Other camera constants
    constexpr uint8_t Ce_MaxCameraCount = 1;
    constexpr uint8_t Ce_MainCameraId = 0;


    // Renderer settings
    constexpr uint32_t DDSCAPS2_CUBEMAP = 0x200;
    constexpr uint32_t DDSCAPS2_VOLUME = 0x200000;
    constexpr uint32_t DDS_DIMENSION_TEXTURE2D = 3;

    constexpr uint32_t Ce_MaxTextureCount = 5'000;
    constexpr uint32_t Ce_MaxMaterialCount = 10'000;

    constexpr uint8_t Ce_MaxLodCountPerSurface = 8;
    constexpr uint32_t Ce_MaxInstanceCountPerLOD = 100'000;
    constexpr uint32_t Ce_MaxInstanceCountPerCluster = 100'000;

    constexpr uint32_t Ce_MaxMeshCount = 1'000'000; 
	constexpr const char* Ce_DefaultMeshName = "bunny";

    constexpr uint32_t Ce_MaxRenderObjects = 5'000'000;
    constexpr uint32_t Ce_MaxONPC_Objects = 100;
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

    #if defined(BLIT_DEPTH_PYRAMID_TEST)
        constexpr uint32_t Ce_DepthPyramidDebug = 1;
    #else
        constexpr uint32_t Ce_DepthPyramidDebug = 0;
    #endif

    #if defined(_WIN32) && !defined(BLIT_VK_FORCE) && !defined(BLIT_GL_LEGACY_OVERRIDE)
        constexpr bool Ce_HLSL = 1;// Row major?
    #else
        constexpr bool Ce_HLSL = 0;
    #endif
    

    constexpr const char* Ce_BlitzenVersion = "Blitzen Engine";
    constexpr uint32_t Ce_BlitzenMajor = 0;
    constexpr const char* Ce_HostedApp = "Blitzen Game";
    constexpr uint32_t Ce_HostedAppVersion = 1;

    constexpr size_t Ce_LinearAllocatorBlockSize = UINT32_MAX;

    constexpr uint16_t Ce_KeyCallbackCount = 256;

    constexpr uint32_t Ce_WorldContextSystemsCount = 5;

    enum class AllocationType : uint8_t
    {
        DynamicArray = 0,
        Hashmap = 1,
        Queue = 2,
        Bst = 3,
        String = 4,
        Engine = 5,
        Renderer = 6,
        Entity = 7,
        EntityNode = 8,
        Scene = 9,
        SmartPointer = 10,
        LinearAlloc = 11,

        MaxTypes = 12
    };

    enum class AllocationAction : uint8_t
    {
        ALLOC = 0,
        FREE = 1,
        FREE_ALL = 2,

        MAX_ACTIONS
    };

    void ShutdownLogging(size_t totalAllocated, size_t* typeAllocations);

    inline void LogAllocation(AllocationType alloc, size_t size, AllocationAction action)
    {
        static size_t totalAllocated{ 0 };
        size_t typeAllocations[static_cast<size_t>(AllocationType::MaxTypes)]{ 0 };

        if (action == AllocationAction::ALLOC)
        {
            totalAllocated += size;
            typeAllocations[static_cast<uint8_t>(alloc)] += size;
        }
        else if (action == AllocationAction::FREE)
        {
            totalAllocated -= size;
            typeAllocations[static_cast<uint8_t>(alloc)] -= size;
        }
        else if (action == AllocationAction::FREE_ALL)
        {
            ShutdownLogging(totalAllocated, typeAllocations);
        }
    }

    enum class EngineState : uint8_t
    {
        RUNNING = 0,
        SUSPENDED = 1,
        LOADING = 2,
        SHUTDOWN = 3,
        SHUTDOWN_AFTER_LOAD = 5,

        MAX_STATES
    };

    class Engine
    {
    public:
        Engine() = default;
        EngineState m_state{ EngineState::SHUTDOWN };

        // Defined in blitMemory.h
        inline ~Engine()
        {
            LogAllocation(AllocationType::Engine, 0, AllocationAction::FREE_ALL);
        }
    };
}




namespace BlitCL
{
    // Hashmap
    constexpr BlitzenCore::AllocationType MapAlloc = BlitzenCore::AllocationType::Hashmap;

    // SmartPointer
    constexpr BlitzenCore::AllocationType SpnAlloc = BlitzenCore::AllocationType::SmartPointer;

    // String
    constexpr uint8_t ce_blitStringCapacityMultiplier = 2;
    constexpr BlitzenCore::AllocationType StrAlloc = BlitzenCore::AllocationType::String;

    // Dynamic array
    constexpr uint8_t ce_blitDynamiArrayCapacityMultiplier = 2;
    constexpr BlitzenCore::AllocationType DArrayAlloc = BlitzenCore::AllocationType::DynamicArray;
}