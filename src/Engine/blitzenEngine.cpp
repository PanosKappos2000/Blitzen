#include "Core/blitzenEngine.h"
#include "Core/blitTimeManager.h"
// Single file gltf loading https://github.com/jkuhlmann/cgltf
// Placed here because cgltf is called from a template function
#define CGLTF_IMPLEMENTATION
#include "Core/blitEntityManager.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>

namespace BlitzenCore
{
    Engine::Engine() : m_state{EngineState::SHUTDOWN}
    {

    }

    Engine::~Engine()
    {
        // This does absolutely nothing right now
        BlitzenCore::ShutdownLogging();

        BlitzenCore::LogAllocation(BlitzenCore::AllocationType::Engine, 0, BlitzenCore::AllocationAction::FREE_ALL);
    }
}

using BLITZEN_WORLD_CONTEXT = BlitCL::StaticArray<void*, BlitzenCore::Ce_WorldContextSystemsCount>;

using EventSystemMemory = BlitCL::SmartPointer<BlitzenCore::EventSystem>;
using RndResourcesMemory = BlitCL::SmartPointer<BlitzenEngine::RenderingResources, BlitzenCore::AllocationType::Renderer>;
using EntitySystemMemory = BlitCL::SmartPointer<BlitzenCore::GameObjectManager, BlitzenCore::AllocationType::Entity>;


#if defined(BLIT_GDEV_EDT)
int main(int argc, char* argv[])
{
    /* ENGINE SYSTEMS INITIALIZATION*/
    BlitzenCore::Engine engine;
    engine.m_state = BlitzenCore::EngineState::LOADING;

    BlitzenWorld::BlitzenPrivateContext blitzenPrivateContext{};
    BlitzenWorld::BlitzenWorldContext blitzenWorldContext{};
	blitzenPrivateContext.pBlitzenContext = &blitzenWorldContext;

    BlitzenCore::InitLogging();

    BLITZEN_WORLD_CONTEXT ppWORLD_CONTEXT;
    blitzenPrivateContext.pEngineState = &engine.m_state;

    BlitzenEngine::CameraContainer cameraSystem;
    auto& mainCamera = cameraSystem.GetMainCamera();
    BlitzenEngine::SetupCamera(mainCamera);
    blitzenWorldContext.pCameraContainer = &cameraSystem;

    BlitzenCore::WorldTimerManager coreClock;
    blitzenWorldContext.pCoreClock = &coreClock;

    BlitzenEngine::Renderer renderer;
    renderer.Make();
    blitzenPrivateContext.pRenderer = renderer.Data();

    EntitySystemMemory entityManager;
    entityManager.Make(); 
    
    EventSystemMemory eventSystem;
    eventSystem.Make(std::ref(blitzenWorldContext), std::ref(blitzenPrivateContext));

    BLIT_ASSERT(BlitzenPlatform::PlatformStartup(BlitzenCore::Ce_BlitzenVersion, eventSystem.Data(), renderer.Data()));

    BlitzenCore::RegisterDefaultEvents(eventSystem.Data());

    RndResourcesMemory renderingResources;
    renderingResources.Make(renderer.Data());
    blitzenWorldContext.pRenderingResources = renderingResources.Data();


    // Loading resources
    std::mutex mtx;
    std::condition_variable loadingDoneConditional;
    std::atomic<bool> loadingDone(false);
    std::thread loadingThread
    {   [&]() 
        {
             std::lock_guard<std::mutex> lock(mtx);

            if (!BlitzenCore::CreateSceneFromArguments(argc, argv, renderingResources.Data(), renderer.Data(), entityManager.Data()))
            {
                BLIT_FATAL("Failed to allocate resource for requested scene");
                loadingDone = true;
                loadingDoneConditional.notify_one();
                return;
            }
            if (!renderer->SetupForRendering(renderingResources.Data(), mainCamera.viewData.pyramidWidth, mainCamera.viewData.pyramidHeight))
            {
                BLIT_FATAL("Renderer failed to setup, Blitzen's rendering system is offline");
                loadingDone = true;
                loadingDoneConditional.notify_one();
                return;
            }

            loadingDone = true;
            loadingDoneConditional.notify_one();
            engine.m_state = BlitzenCore::EngineState::RUNNING;
        }
    };

    #if(_WIN32)
        loadingThread.detach();
    #else
        loadingThread.join();
    #endif

    // Placeholder loop, waiting to load
    while (engine.m_state == BlitzenCore::EngineState::LOADING)
    {
        coreClock.Update();

        BlitzenPlatform::PlatformPumpMessages();
        eventSystem->UpdateInput(0.f);
        renderer->DrawWhileWaiting(float(coreClock.GetDeltaTime()));
    }

    // Extra setup step needed by dx12
    if (engine.m_state == BlitzenCore::EngineState::RUNNING)
    {
        renderer->FinalSetup();
    }

    // MAIN LOOP
    BlitzenEngine::DrawContext drawContext{ &mainCamera, renderingResources.Data()};
    while(engine.m_state == BlitzenCore::EngineState::RUNNING || engine.m_state == BlitzenCore::EngineState::SUSPENDED)
    {
        if(!BlitzenPlatform::PlatformPumpMessages())
        {
            engine.m_state = BlitzenCore::EngineState::SHUTDOWN;
        }

        if(engine.m_state != BlitzenCore::EngineState::SUSPENDED)
        {
            coreClock.Update();

            UpdateCamera(mainCamera, static_cast<float>(coreClock.GetDeltaTime()));

			entityManager->UpdateDynamicObjects(blitzenWorldContext, blitzenPrivateContext);
            
            renderer->Update(drawContext);
            renderer->DrawFrame(drawContext);
        }

        // Reset window resize, TODO: Why is this here??????
        mainCamera.transformData.bWindowResize = false;

        eventSystem->UpdateInput(coreClock.GetDeltaTime());
    }


    std::unique_lock<std::mutex> lock(mtx);
    loadingDoneConditional.wait(lock, [&] { return loadingDone.load(); });

    // Actual shutdown
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