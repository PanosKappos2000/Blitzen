#include "Core/blitzenEngine.h"
#include "Renderer/Interface/blitRenderer.h"
#include "Core/Dasher/Interface/dasherInterface.h"
#include "Core/Events/blitEvents.h"
#include "Platform/blitPlatformContext.h"
#include "Platform/blitPlatform.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>

using EventSystemMemory = BlitCL::SmartPointer<BlitzenCore::EventSystem>;
using RndResourcesMemory = BlitCL::SmartPointer<BlitzenEngine::RenderingResources, BlitzenCore::AllocationType::Renderer>;
using EntitySystemMemory = BlitCL::SmartPointer<BlitzenEngine::EntityManager, BlitzenCore::AllocationType::Entity>;


#if defined(BLIT_GDEV_EDT)
int main(int argc, char* argv[])
{
    /* ENGINE SYSTEMS INITIALIZATION */
    BlitzenCore::Engine engine;
    engine.m_state = BlitzenCore::EngineState::LOADING;

    BlitzenWorld::BlitzenPrivateContext blitzenPrivateContext{};
    BlitzenWorld::BlitzenWorldContext blitzenWorldContext{};
	blitzenPrivateContext.pBlitzenContext = &blitzenWorldContext;

    BlitzenCore::InitLogging();

    blitzenPrivateContext.pEngineState = &engine.m_state;

    BlitzenEngine::CameraContainer cameraSystem;
    auto& mainCamera = cameraSystem.GetMainCamera();
    BlitzenEngine::SetupCamera(mainCamera);
    blitzenWorldContext.pCameraContainer = &cameraSystem;

    BlitzenCore::WorldTimeManager coreClock;
    blitzenWorldContext.pCoreClock = &coreClock;

    BlitzenEngine::Renderer renderer;
    renderer.Make();
    blitzenPrivateContext.pRenderer = renderer.Data();

    EntitySystemMemory entityManager;
    entityManager.Make();
	blitzenPrivateContext.pEntityMangager = entityManager.Data();
    
    EventSystemMemory eventSystem;
    eventSystem.Make(std::ref(blitzenWorldContext), std::ref(blitzenPrivateContext));

    RndResourcesMemory renderingResources;
    renderingResources.Make();
    blitzenPrivateContext.pRenderingResources = renderingResources.Data();

    BlitzenCore::Dasher dasher;
    blitzenPrivateContext.pDasher = &dasher;

    BlitzenPlatform::PlatformContext platform{};
    blitzenPrivateContext.pPlatform = &platform;

    BlitzenPlatform::PlatformArgs platformArgs{&platform, eventSystem.Data(), renderer.Data(), &dasher};

    BLIT_ASSERT(BlitzenPlatform::SystemStartup(platformArgs));

    BlitzenCore::RegisterDefaultEvents(eventSystem.Data());

    BLIT_ASSERT(RenderingResourcesInit(renderingResources.Data(), renderer.Data()));

    BlitzenEngine::DrawContext drawContext{ mainCamera, renderingResources->m_meshContext, entityManager->m_renderContainer, renderingResources->m_textureManager, &platform};


    // LOADING RESOURCES
    std::mutex mtx;
    std::condition_variable loadingDoneConditional;
    std::atomic<bool> loadingDone(false);
    std::thread loadingThread
    {   
        [&]() 
        {
            std::lock_guard<std::mutex> lock(mtx);

            if (!BlitzenEngine::CreateSceneFromArguments(argc, argv, renderingResources.Data(), renderer.Data(), entityManager.Data()))
            {
                BLIT_FATAL("Failed to allocate resource for requested scene, Blitzen shutting down");
                loadingDone = true;
                loadingDoneConditional.notify_one();
                engine.m_state = BlitzenCore::EngineState::SHUTDOWN;
                return;
            }

            if (!renderer->SetupForRendering(drawContext))
            {
                BLIT_FATAL("Renderer failed to setup, Blitzen shutting down");
                loadingDone = true;
                loadingDoneConditional.notify_one();
                engine.m_state = BlitzenCore::EngineState::SHUTDOWN;
                return;
            }

            loadingDone = true;
            loadingDoneConditional.notify_one();
            engine.m_state = BlitzenCore::EngineState::SETUP_AFTER_LOAD;
        }
    };

    #if(_WIN32)

        loadingThread.detach();

    #else

        loadingThread.join();

    #endif

    // LOOP
    while(engine.m_state != BlitzenCore::EngineState::SHUTDOWN)
    {
        if (!BlitzenPlatform::DispatchEvents(&platform))
        {
            engine.m_state = BlitzenCore::EngineState::SHUTDOWN;
        }

        switch (engine.m_state)
        {
        case BlitzenCore::EngineState::RUNNING:
        {
            BlitzenCore::UpdateWorldClock(coreClock);

            BlitzenEngine::UpdateCamera(mainCamera, float(coreClock.m_deltaTime));

            BlitzenEngine::UpdateDynamicObjects(renderer.Data(), entityManager.Data(), blitzenWorldContext);

            renderer->DrawFrame(drawContext);

#if defined(DASHER_JOIN)

            dasher.Draw((float)coreClock.m_deltaTime);

#endif
            renderer->Present();

            break;
        }
        case BlitzenCore::EngineState::LOADING:
        {
            BlitzenCore::UpdateWorldClock(coreClock);

            renderer->DrawWhileWaiting(float(coreClock.m_deltaTime));

            break;
        }
        case BlitzenCore::EngineState::SETUP_AFTER_LOAD:
        {
            renderer->FinalSetup();

            engine.m_state = BlitzenCore::EngineState::RUNNING;

            break;
        }
        case BlitzenCore::EngineState::SUSPENDED:
        {
            break;
        }
        case BlitzenCore::EngineState::MAX_STATES:
        default:
        {
            engine.m_state = BlitzenCore::EngineState::SHUTDOWN;
            break;
        }
        }

        eventSystem->UpdateInput(coreClock.m_deltaTime);
    }


    std::unique_lock<std::mutex> lock(mtx);
    loadingDoneConditional.wait(lock, [&] { return loadingDone.load(); });
}

#else

// this was supposed to test my string but I have forgotten about it
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