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

    constexpr int64_t CE_BLITZEN_FATAL = -1000;
    constexpr int64_t CE_BLITZEN_SUCCESS = 0;

    enum class LogLevel : int8_t
    {
        FATAL = -100,
        ERR = -10,

        INFO = 1,
        WARN = 2,
        DEBUG = 3,
        TRACE = 4,

        SUCCESS = 0
    };

    #define BLIT_ARRAY_SIZE(array)   sizeof(array) / sizeof(array[0])

    bool BLIT_CHECK_SUCCESS(int64_t code);

    bool BLIT_CHECK_FAIL(int64_t code);

    bool BLIT_CHECK_FATAL(int64_t code);

    bool LOG_ERROR_MSG_AND_RETURN(const char* system, const char* msg);

    using BIG_BOOL = uint16_t;
    constexpr BIG_BOOL BB_TRUE = 1;
    constexpr BIG_BOOL BB_FALSE = 0;

    using FAT_BOOL = uint32_t;
	constexpr FAT_BOOL FAT_TRUE = 1;
	constexpr FAT_BOOL FAT_FALSE = 0;

    template<typename PTR>
    using ARRAY_OF_POINTERS = PTR**;

    constexpr size_t ARRAY_SIZE_PLACEHOLDER = 100;

    constexpr double CE_MAX_TIME_STEP = 0.1f;

#if !defined(BLIT_VK_FORCE)

#undef DASHER_JOIN

#endif

    /********************************************************************************************************************************************************
    * SECTION: BLITZEN ENGINE SYSTEM CONSTANTS                                                                                                              *
    *********************************************************************************************************************************************************/
    constexpr const char* CE_LOGGER_SYSTEM_NAME = "blit_logger";
    constexpr const char* CE_MEMORY_SYSTEM_NAME = "blit_mem";
    constexpr const char* CE_PLATFORM_SYSTEM_NAME = "blit_platform";
    constexpr const char* CE_MESH_SYSTEM_NAME = "MeshResources";
    constexpr const char* CE_MESH_DATA_GENERATOR_SYSTEM_NAME = "blit_mesh_data_generator";
    constexpr const char* CE_RESOURCE_SYSTEM_NAME = "RenderingResourceSystem";
    constexpr const char* CE_WORLD_SYSTEM_NAME = "WORLD";
    constexpr const char* CE_SCENE_SYSTEM_NAME = "SceneManager";
    constexpr const char* CE_RESIDENT_SYSTEM_NAME = "WORLD_RESIDENTS";
    constexpr const char* CE_WORLD_VARIABLE_SYSTEM_NAME = "WorldVariables";
    constexpr const char* CE_VULKAN_SYSTEM_NAME = "BlitzenVulkan";
    constexpr const char* CE_DX12_SYSTEM_NAME = "BlitzenDX12";
    constexpr const char* CE_RENDERER_SYSTEM_NAME = "blit_renderer";
    constexpr const char* CE_DASHER_EDITOR_SYSTEM_NAME = "dasher_editor";
    constexpr const char* CE_DEAR_DASHER_EDITOR_SYSTEM_NAME = "dearDasher_editor";
	constexpr const char* CE_BLITZEN_LOADING_LOOP_NAME = "BLITZEN_LOADING_LOOP";

    constexpr uint32_t CE_MESSAGE_BUFFER_SIZE = 1500;

	constexpr size_t Ce_BlitLogOutputFileSize = 1024 * 1024 * 10; // 10 MB
    constexpr size_t CE_RAPID_RESOURCE_FILE_SIZE = 1024 * 1024 * 10; // 10 MB
    constexpr size_t CE_TEXTURE_DATA_HANDLE_SIZE = 128 * 1024 * 1024;

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
        WV = 12,
        TRIANGLE = 13,
        Texture = 14,

        MaxTypes = 100
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

    constexpr const char* Ce_PrimaryGltfTestScene = "../../GltfTestScenes/Scenes/Plaza/scene.gltf";

    /********************************************************************************************************************************************************
    * SECTION: RENDERING RESOURCES CONSTANTS                                                                                                                *
    *********************************************************************************************************************************************************/
    constexpr uint32_t DDSCAPS2_CUBEMAP = 0x200;
    constexpr uint32_t DDSCAPS2_VOLUME = 0x200000;
    constexpr uint32_t DDS_DIMENSION_TEXTURE2D = 3;

    constexpr uint32_t Ce_MaxTextureCount = 5'000;
    constexpr const char* Ce_DefaultTextureName = "BlitzenReindeer";

    constexpr uint32_t Ce_MaxMaterialCount = 10'000;
	constexpr const char* Ce_DefaultMaterialName = "BlitzenReindeerAlbedoMaterial";

    constexpr uint32_t CE_TRIANGLE_VERTICES = 3;
    constexpr uint32_t Ce_MaxWorldVertexCount = 30'000'000;
    constexpr uint32_t Ce_MaxWorldVertexIndicesCount = Ce_MaxWorldVertexCount * CE_TRIANGLE_VERTICES;
    static_assert(Ce_MaxWorldVertexCount <= Ce_MaxWorldVertexIndicesCount);

    constexpr uint32_t Ce_MaxMeshCount = 10'000;
    constexpr uint32_t Ce_EngineDefaultMeshesCount = 4;
    constexpr const char* Ce_DefaultMeshName = "bunny";
    constexpr const char* Ce_DefaultDragonMeshName = "Stanford Dragon";
    constexpr const char* Ce_DefaultHumanMeshname = "Base Human";
    constexpr const char* Ce_DefaultKittenMeshName = "Kitten";
    constexpr const char* Ce_MeshDoNotAddToTable = "BLIT_DO_NOT_ADD_TO_MESH_TABLE";

    constexpr uint32_t Ce_MaxMeshPrimitivesCount = 10'000;

    constexpr uint8_t Ce_MaxLodCountPerSurface = 8;

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

#if defined(BLITZEN_DRAW_DOUBLE_PASS_OCCLUSION)
    constexpr uint8_t CE_OCCLUSION_DOUBLE_PASS = 1;
#else
    constexpr uint8_t CE_OCCLUSION_DOUBLE_PASS = 0;
#endif

    enum class BLIT_RENDERING_SYSTEM : uint32_t
    {
        BLITZEN_VULKAN,
        BLITZEN_DX12,
        BLITZEN_GL,
    };
    constexpr BLIT_RENDERING_SYSTEM CE_BLITZEN_RENDERING_SYSTEM = BLIT_RENDERING_SYSTEM::BLITZEN_DX12;
#if defined(linux)
    static_assert(CE_BLITZEN_RENDERING_SYSTEM == BLIT_RENDERING_SYSTEM::BLITZEN_VULKAN);
#endif


    void ShutdownLogging(size_t totalAllocated, size_t* typeAllocations);

    void LogAllocation(AllocationType alloc, size_t size, AllocationAction action);
    
    enum class EngineState : uint8_t
    {
        RUNNING = 0,
        RUNNING_EDITOR_NO_START = 5,

        SUSPENDED = 20,
        LOADING = 21,
        SETUP_AFTER_LOAD = 22,
        STARTUP = 23,

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

    /********************************************************************************************************************************************************
    * SECTION: ENGINE STATE MANAGER AND ALLOCATION LOGGER                                                                                                   *
    *********************************************************************************************************************************************************/
    class Engine
    {
    public:
        Engine() = default;
        volatile EngineState m_state{ EngineState::SHUTDOWN };

        // Defined in blitMemory.h
        ~Engine();
    };
}

#define BLIT_FAT_FALSE BlitzenCore::FAT_FALSE;
#define BLIT_FAT_TRUE BlitzenCore::FAT_TRUE;

#define BLIT_BB_TRUE BlitzenCore::BB_TRUE;
#define BLIT_BB_FALSE BlitzenCore::BB_FALSE;

using BLIT_STRAIGHTHANDLE = void*;

/********************************************************************************************************************************************************
* SECTION: CONTAINER LIBRARY CONSTANTS                                                                                                                  *
*********************************************************************************************************************************************************/
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