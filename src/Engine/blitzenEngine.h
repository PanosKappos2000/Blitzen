#pragma once

#include "Core/blitAssert.h"
#include "Platform/platform.h"
#include "Core/blitzenContainerLibrary.h"
#include "Core/blitEvents.h"
#include "BlitzenMathLibrary/blitMLTypes.h"
#include "Game/blitCamera.h"


#define BLITZEN_VERSION                 "Blitzen Engine 0"

// Should graphics API implementation be loaded
#define BLITZEN_VULKAN                  1
#define BLITZEN_DIRECTX12               0

// Window macros
#define BLITZEN_WINDOW_STARTING_X       100
#define BLITZEN_WINDOW_STARTING_Y       100
#define BLITZEN_WINDOW_WIDTH            1280
#define BLITZEN_WINDOW_HEIGHT           768
#define BLITZEN_ZNEAR                   0.01f
#define BLITZEN_FOV                     BlitML::Radians(70.f)
#define BLITZEN_DRAW_DISTANCE           600.f

// Various hardcoded modes to test the renderer
#define BLITZEN_GEOMETRY_STRESS_TEST        1
#define BLITZEN_GLTF_SCENE                  1
#define BLITZEN_OBJ_AND_GLTF_COMBINE_TEST   1

namespace BlitzenEngine
{
    struct PlatformStateData
    {
        uint32_t windowWidth = BLITZEN_WINDOW_WIDTH;
        uint32_t windowHeight = BLITZEN_WINDOW_HEIGHT;
        uint8_t windowResize = 0;
    };

    struct Clock
    {
        double startTime = 0;
        double elapsedTime = 0;
    };

    struct EngineSystems
    {
        uint8_t engine = 0;

        uint8_t eventSystem = 0;
        // The event state is owned by the engine so that the dynamic array inside it, get freed before memory management is shutdown
        BlitzenCore::EventSystemState eventState;

        uint8_t inputSystem = 0;
        // The input state is placed here, so that its memory can be managed by the engine
        BlitzenCore::InputState inputState;

        uint8_t logSystem = 0;
    };

    class Engine
    {
    public:

        
        Engine(); 

        void Run();

        //~Engine(); Same thing for constructor also goes for destructor
        void Shutdown();


        inline void RequestShutdown() { isRunning = 0; }

        inline static Engine* GetEngineInstancePointer() { return pEngineInstance; }

        inline EngineSystems& GetEngineSystems() { return m_systems; }

        void UpdateWindowSize(uint32_t newWidth, uint32_t newHeight);

        // Returns the main camera
        inline Camera& GetCamera() { return m_mainCamera; }
        // Returns the moving camera (in some cases, like detachment, it is differen than the main camera)
        inline Camera* GetMovingCamera() { return m_pMovingCamera; }
        // Change the moving camera
        inline void SetMovingCamera(Camera* pCamera) { m_pMovingCamera = pCamera; }
        // Return the camera container to get access to all the available cameras
        inline CameraContainer& GetCameraContainer() { return m_cameraContainer; }

        // Returns delta time, crucial for functions that move objects
        inline double GetDeltaTime() { return m_deltaTime; }

        inline void ChangeFreezeFrustumState() { m_freezeFrustum = !m_freezeFrustum; }
        inline void ChangeDebugPyramidActiveState() { isDebugPyramidActive = !isDebugPyramidActive; }
        inline void ChangeOcclusionCullingState() { m_occlusionCulling = !m_occlusionCulling; }
        inline void ChangeLodEnabledState()  { m_lodEnabled = !m_lodEnabled; }

    private:

        // Makes sure that the engine is only created once and gives access subparts of the engine through static getter
        static Engine* pEngineInstance;

        uint8_t isRunning = 0;
        uint8_t isSupended = 0;
        
        PlatformStateData m_platformData;

        Clock m_clock;
        double m_deltaTime;// The clock helps the engine keep track of the delta time

        EngineSystems m_systems;

        // Holds all the camera created and an index to the active one
        CameraContainer m_cameraContainer;

        // The main camera is the one whose values are used for culling and other operations
        Camera& m_mainCamera;
        // The camera that moves around the scene, usually the same as the main camera, unless the user requests detatch
        Camera* m_pMovingCamera;

        // Debug values, these should probably be held by the renderer and not the engine
        uint8_t m_freezeFrustum = 0;
        uint8_t isDebugPyramidActive = 0;
        uint8_t m_occlusionCulling = 1;
        uint8_t m_lodEnabled = 1;
    };

    void RegisterDefaultEvents();

    // Will be registered to the event system on initalization
    uint8_t OnEvent(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);
    uint8_t OnKeyPress(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);
    uint8_t OnResize(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);
    uint8_t OnMouseMove(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);
}
