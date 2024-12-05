#pragma once

#include "Core/blitAssert.h"
#include "Platform/platform.h"
#include "Core/blitzenContainerLibrary.h"
#include "Core/blitEvents.h"
#include "Application/textures.h"
#include "BlitzenMathLibrary/blitMLTypes.h"


#define BLITZEN_VERSION                 "Blitzen Engine 0.C"

#define BLITZEN_VULKAN                  1

#define BLITZEN_WINDOW_STARTING_X       100
#define BLITZEN_WINDOW_STARTING_Y       100
#define BLITZEN_WINDOW_WIDTH            1280
#define BLITZEN_WINDOW_HEIGHT           720

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

        uint8_t logSystem = 0;

        #if BLITZEN_VULKAN
            uint8_t vulkan = 0;
        #endif
    };

    // Temporary camera struct, this will have its own file and will be a robust system
    struct Camera
    {
        uint8_t cameraDirty = 0;

        BlitML::mat4 viewMatrix;
        BlitML::mat4 projectionMatrix;
        BlitML::mat4 projectionViewMatrix;

        BlitML::vec3 position;
        BlitML::vec3 eulerAngles;

        BlitML::vec3 velocity = BlitML::vec3(0.f);
    };

    enum class ActiveRenderer : uint8_t
    {
        Vulkan = 0,

        MaxRenderers = 1
    };

    class Engine
    {
    public:
        Engine();

        void Run();

        // I might want to explicitly shutdown the engine, as there are some systems that are initalized outside of it
        ~Engine();
        inline void RequestShutdown() { isRunning = 0; }

        inline static Engine* GetEngineInstancePointer() { return pEngineInstance; }

        inline EngineSystems& GetEngineSystems() { return m_systems; }

        void UpdateWindowSize(uint32_t newWidth, uint32_t newHeight);

        inline Camera& GetCamera() { return m_camera; }

    private:

        void StartClock();
        void StopClock();

        /* ---------------------------------------------------
            Dedicated function for the renderers. 
            If Blitzen gets more than one renderer, 
            this will decide which ones will be initialized 
            and which one will draw the frame.
        -------------------------------------------------------*/
        uint8_t RendererInit();
        void SetupForRenderering();
        void DrawFrame();
        void RendererShutdown();

    private:

        // Makes sure that the engine is only created once and gives access subparts of the engine through static getter
        static Engine* pEngineInstance;

        uint8_t isRunning = 0;
        uint8_t isSupended = 0;

        // Used to create the window and other platform specific part of the engine
        BlitzenPlatform::PlatformState m_platformState;
        PlatformStateData m_platformData;

        Clock m_clock;

        ActiveRenderer m_renderer;

        EngineSystems m_systems;

        Camera m_camera;

        // Temporary to ready my renderer and then I will add more robust systems
        TextureStats m_textures[1000];
    };

    // Will be registered to the event system on initalization
    uint8_t OnEvent(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);
    uint8_t OnKeyPress(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);
    uint8_t OnResize(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data);

    // This will be moved somewhere else once I have a good camera system
    void UpdateCamera(Camera& camera, float deltaTime);
}
