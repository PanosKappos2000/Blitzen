#pragma once

#include "Core/blitAssert.h"
#include "Platform/platform.h"
#include "Core/blitzenContainerLibrary.h"
#include "Core/blitEvents.h"
#include "Renderer/blitRenderingResources.h"
#include "BlitzenMathLibrary/blitMLTypes.h"


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

// Define what Blitzen tries to do at runtime. 
// The first one tries to draw a heavy scene with multiple meshes. 
// The second one tests physics implementation
// Both are tests and now the way the engine should normally work
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

    // Temporary camera struct, this will have its own file and will be a robust system
    struct Camera
    {
        uint8_t cameraDirty = 0;// Tells the engine if the camera should be updated

        BlitML::mat4 viewMatrix;
        BlitML::mat4 projectionMatrix;
        BlitML::mat4 projectionViewMatrix;
        // Needed for frustum culling (probably does not need to be managed by the camera)
        BlitML::mat4 projectionTranspose;

        BlitML::vec3 position;
        // These 2 should be enough to keep track of rotation 
        // (A quat and rotation matrix will be created on the fly, to contribute to the final view matrix)
        float yawRotation = 0.f;
        float pitchRotation = 0.f;

        BlitML::mat4 rotation = BlitML::mat4();
        BlitML::mat4 translation = BlitML::mat4();

        BlitML::vec3 velocity = BlitML::vec3(0.f);
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
        inline EngineResources& GetEngineResources() { return m_resources; }

        void UpdateWindowSize(uint32_t newWidth, uint32_t newHeight);

        inline Camera& GetCamera() { return m_camera; }
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

        Camera m_camera;
        
        EngineResources m_resources;
    };

    // Will be registered to the event system on initalization
    uint8_t OnEvent(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);
    uint8_t OnKeyPress(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);
    uint8_t OnResize(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);
    uint8_t OnMouseMove(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);

    // This will be moved somewhere else once I have a good camera system
    void UpdateCamera(Camera& camera, float deltaTime);
    void RotateCamera(Camera& camera, float deltaTime, float pitchRotation, float yawRotation);
}
