#include "Engine/blitzenEngine.h"
// Single file gltf loading https://github.com/jkuhlmann/cgltf
// Placed here because cgltf is called from a template function
#define CGLTF_IMPLEMENTATION
#include "Platform/platform.h"
#include "Renderer/blitRenderer.h"
#include "Core/blitTimeManager.h"
#include <thread>
#include <iostream>

namespace BlitzenEngine
{
    // Function that is defined in blitzenDefaultEvents.cpp and only called once in main
    void RegisterDefaultEvents();

    Engine* Engine::s_pEngine = nullptr;

    Engine::Engine() : m_bRunning{ true }, m_bSuspended{ true }
    {
        BLIT_ASSERT(!s_pEngine);
        s_pEngine = this;
    }

    Engine::~Engine()
    {
        // This does absolutely nothing right now
        BlitzenCore::ShutdownLogging();
    }
}

using EventSystem = BlitCL::SmartPointer<BlitzenCore::EventSystemState>;
using InputSystem = BlitCL::SmartPointer<BlitzenCore::InputSystemState>;
using PRenderingResources = BlitCL::SmartPointer<BlitzenEngine::RenderingResources, BlitzenCore::AllocationType::Renderer>;
using Entities = BlitCL::SmartPointer<BlitzenEngine::GameObjectManager, BlitzenCore::AllocationType::Entity>;

static void LoadRenderingResources(int argc, char** argv,
    BlitzenEngine::RenderingResources* pResources, BlitzenEngine::RendererPtrType pRenderer,
    BlitzenEngine::GameObjectManager* pManager, BlitzenEngine::Camera& camera)
{
    BlitzenEngine::CreateSceneFromArguments(argc, argv, pResources,
        pRenderer, pManager);
    if (!pRenderer->SetupForRendering(pResources, camera.viewData.pyramidWidth, camera.viewData.pyramidHeight))
    {
        BLIT_FATAL("Renderer failed to setup, Blitzen's rendering system is offline");
        return;
    }

    BlitzenEngine::Engine::GetEngineInstancePointer()->ReActivate();
}


#if defined(BLIT_GDEV_EDT)
int main(int argc, char* argv[])
{
    /*
        Engine Systems initialization
    */
    BlitzenEngine::Engine engine;
    BlitzenCore::InitLogging();
    
    // Memory manager, needs to be created before everything that uses heap allocations
    BlitzenCore::MemoryManagerState blitzenMemory;
#if !defined(BLITZEN_VULKAN_OVERRIDE)
    BlitzenVulkan::MemoryCrucialHandles memoryCrucials;
#endif
    
    // Platform depends on input and event system, after they're both ready default events are registered
    EventSystem eventSystemState;
	eventSystemState.Make();
    InputSystem inputSystemState;
    inputSystemState.Make();

    BlitzenEngine::Renderer renderer;
    renderer.Make();
    BLIT_ASSERT(BlitzenPlatform::PlatformStartup(BlitzenEngine::ce_blitzenVersion,
        eventSystemState.Data(), inputSystemState.Data(), renderer.Data()));

    BlitzenEngine::RegisterDefaultEvents();

    BlitzenEngine::CameraSystem cameraSystem;
    auto& mainCamera = cameraSystem.GetCamera();
    BlitzenEngine::SetupCamera(mainCamera);
   
    PRenderingResources renderingResources;
    renderingResources.Make(renderer.Data());

    Entities entities;
	entities.Make();

    // Loading resources
    std::thread loadingThread{ [&]() {
        LoadRenderingResources(argc, argv, renderingResources.Data(), renderer.Data(),
            entities.Data(), mainCamera);
    }};
    loadingThread.detach();


    // Placeholder loop, waiting to load
    BlitzenCore::WorldTimerManager coreClock;
    while (!engine.IsActive() && engine.IsRunning())
    {
        BlitzenPlatform::PlatformPumpMessages();
        inputSystemState->UpdateInput(0.f);
        renderer->DrawWhileWaiting();
    }

    // Main loop
    BlitzenEngine::DrawContext drawContext{ &mainCamera, renderingResources.Data()};
    while(engine.IsRunning())
    {
        // Captures events
        if(!BlitzenPlatform::PlatformPumpMessages())
        {
            engine.Shutdown();
        }

        if(engine.IsActive())
        {
            // Update delta time
            coreClock.Update();

            // Updates camera
            UpdateCamera(mainCamera, static_cast<float>(coreClock.GetDeltaTime()));

            // Updates dynamic entities
			entities->UpdateDynamicObjects();
            
            // Render
            renderer->SetupWhileWaitingForPreviousFrame(drawContext);
            renderer->DrawFrame(drawContext);
        }

        // Reset window resize, TODO: Why is this here??????
        mainCamera.transformData.bWindowResize = false;

        inputSystemState->UpdateInput(coreClock.GetDeltaTime());
    }

	// Wait for the loading thread to finish (Got to fix this shit)
    if (!engine.IsActive())
    {
        BLIT_ERROR("Wait for resource loading");
        BlitzenPlatform::PlatformSleep(100'000);// Wait for 100 seconds because I am too lazy to create a mutex for this (idiot)
    }

    // Actual shutdown
    engine.BeginShutdown();
    BlitzenPlatform::PlatformShutdown();
}


#else
int main()
{
    BlitzenCore::MemoryManagerState blitzenMemory;
    BlitCL::String string{ "Trying out my string class" };

    /*cgltf_options options = {};
    cgltf_data* pData = nullptr;
    auto path = "../../GltfTestScenes/Scenes/Plaza/scene.gltf";
    auto res = cgltf_parse_file(&options, path, &pData);
    // Automatic free struct
    struct CgltfScope
    {
        cgltf_data* pData;
        inline ~CgltfScope() { cgltf_free(pData); }
    };
    CgltfScope cgltfScope{ pData };
    res = cgltf_load_buffers(&options, pData, path);
    res = cgltf_validate(pData);

    BlitCL::DynamicArray<std::string> textures{pData->textures_count};

        for (size_t i = 0; i < pData->textures_count; ++i)
        {
            auto pTexture = &(pData->textures[i]);
            if (!pTexture->image)
                break;

            auto pImage = pTexture->image;
            if (!pImage->uri)
                break;

            std::string ipath = path;
            auto pos = ipath.find_last_of('/');
            if (pos == std::string::npos)
                ipath = "";
            else
                ipath = ipath.substr(0, pos + 1);

            std::string uri = pImage->uri;
            uri.resize(cgltf_decode_uri(&uri[0]));
            auto dot = uri.find_last_of('.');

            if (dot != std::string::npos)
                uri.replace(dot, uri.size() - dot, ".dds");

            auto path = ipath + uri;

            textures[i] = path;
        }*/

    BLIT_TRACE(string.GetClassic());

    BLIT_TRACE("capacity %i", string.GetCapacity());
    BLIT_TRACE("size %i", string.GetSize());

    string.Append("Append a long string so that I can invoke the IncreaseCapacity function");
    BLIT_TRACE(string.GetClassic());

    BlitCL::String otherString{ "Lets see what this can do. Leave some space" };
    BLIT_TRACE("Char: %c", otherString[otherString.FindLastOf('.')]);

        BlitCL::String sub{ otherString.Substring(0, 10) };
	BLIT_TRACE("Sub: %s", sub.GetClassic());

    otherString.ReplaceSubstring(20, "Blitzen");
	BLIT_TRACE("Replaced: %s", otherString.GetClassic());

    //BLIT_ASSERT(false)
}
#endif


//Assets/Scenes/CityLow/scene.gltf ../../GltfTestScenes/Scenes/Plaza/scene.gltf ../../GltfTestScenes/Scenes/Museum/scene.gltf (personal test scenes for copy pasting)