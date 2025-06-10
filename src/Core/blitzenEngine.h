#pragma once
#include <stdio.h>
#include <cstdint>
#include <cstddef>

namespace BlitzenCore
{
    constexpr const char* CE_BLITZEN = "Blitzen.v0";
    constexpr uint32_t Ce_BlitzenMajor = 0;
    constexpr const char* Ce_HostedApp = "Blitzen Client";
    constexpr uint32_t Ce_HostedAppVersion = 1;

    // Window constants
    constexpr uint32_t Ce_WindowStartingX = 100;
    constexpr uint32_t Ce_WindowStartingY = 100;
    constexpr uint32_t Ce_InitialWindowWidth = 1280;
    constexpr uint32_t Ce_InitialWindowHeight = 720;
    constexpr float Ce_DefaultWindowBackgroundColor[4] = { 0.f, 0.2f, 0.4f, 1.f };

    enum class LogLevel : int8_t
    {
        FATAL = -20,
        ERR = -10,

        INFO = 1,
        WARN = 2,
        DEBUG = 3,
        TRACE = 4,

        SUCCESS = 0
    };

    template<typename T>
    bool BLIT_CHECK_SUCCESS(T code)
    {
        return code == T::SUCCESS;
    }

    template<typename T>
    bool BLIT_CHECK_FAIL(T code)
    {
        return code < T::SUCCESS;
    }

    using BIG_BOOL = uint16_t;

#if !defined(BLIT_VK_FORCE)

#undef DASHER_JOIN

#endif

    constexpr uint32_t CE_MESSAGE_BUFFER_SIZE = 1500;

	constexpr size_t Ce_BlitLogOutputFileSize = 1024 * 1024 * 10; // 10 MB

    constexpr uint32_t Ce_MaxControllerCount = 5;

    constexpr uint32_t Ce_EditorControllerID = 0;
    constexpr uint32_t Ce_EngineDefaultGameControllerID = 1;

#if defined(DASHER_JOIN)
    constexpr uint32_t Ce_InitialControllerID = Ce_EditorControllerID;
#else
    constexpr uint32_t Ce_InitialControllerID = Ce_EngineDefaultGameControllerID;
#endif

    constexpr uint16_t Ce_KeyCallbackCount = 256;
    constexpr uint16_t Ce_MouseButtonPFNCount = 3;

    constexpr uint32_t Ce_EditorEventQueueSize = 10;
    constexpr uint32_t Ce_EditorButtonEventTypeCount = 5;

    constexpr uint32_t Ce_ImguiFreezeFrustumButtonID = 0;
    constexpr uint32_t Ce_ImguiSceneStartButtonID = 1;
    constexpr uint32_t Ce_ImguiDebugWindowCloseID = 2;

    constexpr size_t Ce_LinearAllocatorBlockSize = UINT32_MAX;

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
    constexpr const char* Ce_DefaultTextureName = "BlitzenReindeer";

    constexpr uint32_t Ce_MaxMaterialCount = 10'000;
	constexpr const char* Ce_DefaultMaterialName = "BlitzenReindeerAlbedoMaterial";


    constexpr uint32_t Ce_MaxMeshCount = 10'000;
    constexpr uint32_t Ce_EngineDefaultMeshesCount = 4;
    constexpr const char* Ce_DefaultMeshName = "bunny";
    constexpr const char* Ce_DefaultDragonMeshName = "Stanford Dragon";
    constexpr const char* Ce_DefaultHumanMeshname = "Base Human";
    constexpr const char* Ce_DefaultKittenMeshName = "Kitten";
    constexpr const char* Ce_MeshDoNotAddToTable = "BLIT_DO_NOT_ADD_TO_MESH_TABLE";

    constexpr uint32_t Ce_MaxMeshPrimitivesCount = 10'000;

    constexpr uint8_t Ce_MaxLodCountPerSurface = 8;
    constexpr uint32_t Ce_MaxLODs = Ce_MaxMeshPrimitivesCount * Ce_MaxLodCountPerSurface;
    constexpr uint32_t Ce_MaxInstanceCountPerLOD = 100'000;

    // CE_MAX_CLUSTER_PER_SURFACE ?????
    constexpr uint32_t Ce_MaxClusters = 1'000;
    constexpr uint32_t Ce_MaxInstanceCountPerCluster = 100'000;

    constexpr uint32_t Ce_MaxVerticesPerCluster = 64;
    constexpr uint32_t Ce_MaxTrianglesPerCluster = 124;
    constexpr float Ce_ClusterConeWeight = 0.25f;

    constexpr uint32_t Ce_MaxRenderObjects = 5'000'000;
    constexpr uint32_t Ce_MaxTransparentRenderObjects = 100'000;
    constexpr uint32_t Ce_MaxONPC_Objects = 100;

    constexpr uint32_t Ce_MaxDynamicObjectCount = 1'000;

    constexpr uint32_t Ce_MaxWorldCollisionGridCount = 100'000;
    constexpr uint32_t Ce_MaxCollisionsInGrid = 50;
    constexpr uint32_t Ce_CollisionGridDynamicOffset = 40;

    constexpr uint32_t Ce_MaxWorldVariableCount = 1'000;

#if defined(BLIT_DYNAMIC_OBJECT_TEST)

    constexpr uint8_t Ce_LoadDynamicObjectTest = 1;

#else

    constexpr uint8_t Ce_LoadDynamicObjectTest = 0;

#endif

#if defined(BLITZEN_CLUSTER_CULLING)

    constexpr uint8_t Ce_BuildClusters = 1;

#else
        
    constexpr uint8_t Ce_BuildClusters = 0;

#endif

#if defined(BLITZEN_DRAW_TEMPORAL_OCCLUSION)

    constexpr uint8_t Ce_DrawTemporalOcclusion = 1;

#else

    constexpr uint8_t Ce_DrawTemporalOcclusion = 0;

#endif

#if defined(BLITZEN_DRAW_INSTANCED_CULLING)
        
    constexpr uint8_t Ce_InstanceCulling = 1;
    
#else
    
    constexpr uint8_t Ce_InstanceCulling = 0;
    
#endif

#if defined(BLIT_DEPTH_PYRAMID_TEST)

    constexpr uint32_t Ce_DepthPyramidDebug = 1;

#else
    
    constexpr uint32_t Ce_DepthPyramidDebug = 0;

#endif

#if defined(BLIT_DEPTH_PYRAMID_TEST) || defined(BLITZEN_DRAW_TEMPORAL_OCCLUSION) || defined(BLITZEN_CLUSTER_CULLING)

    constexpr uint8_t Ce_OcclusionCulling = 1;
    constexpr uint8_t Ce_Build_HI_Z = 1;

#else

    constexpr uint8_t Ce_OcclusionCulling = 0;
    constexpr uint8_t Ce_Build_HI_Z = 0;

#endif


    void ShutdownLogging(size_t totalAllocated, size_t* typeAllocations);

    inline void LogAllocation(AllocationType alloc, size_t size, AllocationAction action)
    {
        static size_t totalAllocated{ 0 };
        static size_t typeAllocations[size_t(AllocationType::MaxTypes)]{ 0 };

        if (action == AllocationAction::ALLOC)
        {
            totalAllocated += size;
            typeAllocations[uint8_t(alloc)] += size;
        }
        else if (action == AllocationAction::FREE)
        {
            totalAllocated -= size;
            typeAllocations[uint8_t(alloc)] -= size;
        }
        else if (action == AllocationAction::FREE_ALL)
        {
            ShutdownLogging(totalAllocated, typeAllocations);
        }
    }

    enum class EngineState : uint8_t
    {
        RUNNING = 0,
        RUNNING_EDITOR_NO_START = 5,

        SUSPENDED = 20,
        LOADING = 21,
        SETUP_AFTER_LOAD = 22,

        SHUTDOWN = 127,
        SHUTDOWN_AFTER_LOAD = 128,

        MAX_STATES
    };

#if defined(DASHER_JOIN)

    constexpr bool Ce_BlitEditorMode = 1;
    constexpr EngineState Ce_InitialEngineRunningState = EngineState::RUNNING;

#else

    constexpr bool Ce_BlitEditorMode = 0;
    constexpr EngineState Ce_InitialEngineRunningState = EngineState::RUNNING_EDITOR_NO_START;

#endif

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