#pragma once

#include "Core/blitAssert.h"
#include "Platform/platform.h"
#include "Core/blitzenContainerLibrary.h"
#include "Core/blitEvents.h"
#include "Renderer/blitRenderingResources.h"
#include "BlitzenMathLibrary/blitMLTypes.h"
#include "Game/blitCamera.h"


#define BLITZEN_VERSION                 "Blitzen Engine 0.C"

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

#define BLITZEN_GRAPHICS_STRESS_TEST    1
#define BLITZEN_PHYSICS_TEST            0

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

        #if BLITZEN_VULKAN
            uint8_t vulkan = 0;
        #endif

        #if BLITZEN_DIRECTX12
            uint8_t directx12 = 0;
        #endif
    };

    enum class ActiveRenderer : uint8_t
    {
        Vulkan = 0,
        Directx12 = 1,

        MaxRenderers = 2
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
        inline RenderingResources& GetEngineResources() { return m_resources; }

        void UpdateWindowSize(uint32_t newWidth, uint32_t newHeight);

        // Returns the main camera
        inline Camera& GetCamera() { return m_mainCamera; }
        // Returns the moving camera (in some cases, like detachment, it is differen than the main camera)
        inline Camera* GetMovingCamera() { return m_pMovingCamera; }
        // Change the moving camera
        inline void SetMovingCamera(Camera* pCamera) { m_pMovingCamera = pCamera; }
        inline CameraContainer& GetCameraContainer() { return m_cameraContainer; }
        inline double GetDeltaTime() { return m_deltaTime; }

    private:

        void StartClock();
        void StopClock();

    private:

        // Makes sure that the engine is only created once and gives access subparts of the engine through static getter
        static Engine* pEngineInstance;

        uint8_t isRunning = 0;
        uint8_t isSupended = 0;
        
        PlatformStateData m_platformData;

        Clock m_clock;
        double m_deltaTime;// The clock helps the engine keep track of the delta time

        ActiveRenderer m_renderer;

        EngineSystems m_systems;

        // Holds all the camera created and an index to the active one
        CameraContainer m_cameraContainer;

        // The main camera is the one whose values are used for culling and other operations
        Camera& m_mainCamera;
        // The camera that moves around the scene, usually the same as the main camera, unless the user requests detatch
        Camera* m_pMovingCamera;
        
        RenderingResources m_resources;
    };

    // Will be registered to the event system on initalization
    uint8_t OnEvent(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);
    uint8_t OnKeyPress(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);
    uint8_t OnResize(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);
    uint8_t OnMouseMove(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);

    // Implemented in blitzenObject.cpp, define here temporarily to avoid circular dependency
    void CreateTestGameObjects(RenderingResources& resources, uint32_t drawCount);
}
