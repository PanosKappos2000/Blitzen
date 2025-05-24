#include "Core/blitzenEngine.h"
#include "Renderer/Interface/blitRenderer.h"
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
using EntitySystemMemory = BlitCL::SmartPointer<BlitzenCore::EntityManager, BlitzenCore::AllocationType::Entity>;


#if defined(BLIT_GDEV_EDT)
int main(int argc, char* argv[])
{
    /* ENGINE SYSTEMS INITIALIZATION */
    BlitzenCore::Engine engine;
    engine.m_state = BlitzenCore::EngineState::LOADING;

    BlitzenWorld::BlitzenPrivateContext blitzenPrivateContext{};
    BlitzenWorld::BlitzenWorldContext blitzenWorldContext{};
    // Private context points back to world context
	blitzenPrivateContext.pBlitzenContext = &blitzenWorldContext;

    BlitzenCore::InitLogging();

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
	blitzenPrivateContext.pEntityMangager = entityManager.Data();
    
    EventSystemMemory eventSystem;
    eventSystem.Make(std::ref(blitzenWorldContext), std::ref(blitzenPrivateContext));

    RndResourcesMemory renderingResources;
    renderingResources.Make();
    blitzenPrivateContext.pRenderingResources = renderingResources.Data();

    // Platform preferably created last and destroyed first
    BlitzenPlatform::PlatformContext platform{};
    BLIT_ASSERT(BlitzenPlatform::PlatformStartup(BlitzenCore::Ce_BlitzenVersion, &platform, eventSystem.Data(), renderer.Data()));

    BlitzenCore::RegisterDefaultEvents(eventSystem.Data());

    BLIT_ASSERT(RenderingResourcesInit(renderingResources.Data(), renderer.Data()));

    BlitzenEngine::DrawContext drawContext{ mainCamera, renderingResources->m_meshContext, entityManager->m_renderContainer, renderingResources->m_textureManager, &platform};


    // LOADING RESOURCES
    std::mutex mtx;
    std::condition_variable loadingDoneConditional;
    std::atomic<bool> loadingDone(false);
    std::thread loadingThread
    {   [&]() 
        {
             std::lock_guard<std::mutex> lock(mtx);

            if (!BlitzenEngine::CreateSceneFromArguments(argc, argv, renderingResources.Data(), renderer.Data(), entityManager.Data()))
            {
                BLIT_FATAL("Failed to allocate resource for requested scene");
                loadingDone = true;
                loadingDoneConditional.notify_one();
                return;
            }
            // Updates renderer
            if (!renderer->SetupForRendering(drawContext))
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
        BlitzenCore::UpdateWorldClock(coreClock);

        BlitzenPlatform::DispatchEvents(&platform);
        eventSystem->UpdateInput(0.f);
        renderer->DrawWhileWaiting(float(coreClock.m_deltaTime));
    }

    // Extra setup step needed by dx12
    if (engine.m_state == BlitzenCore::EngineState::RUNNING)
    {
        renderer->FinalSetup();
    }

    // MAIN LOOP
    while(engine.m_state == BlitzenCore::EngineState::RUNNING || engine.m_state == BlitzenCore::EngineState::SUSPENDED)
    {
        if(!BlitzenPlatform::DispatchEvents(&platform))
        {
            engine.m_state = BlitzenCore::EngineState::SHUTDOWN;
        }

        if(engine.m_state != BlitzenCore::EngineState::SUSPENDED)
        {
            BlitzenCore::UpdateWorldClock(coreClock);

            BlitzenEngine::UpdateCamera(mainCamera, float(coreClock.m_deltaTime));

			BlitzenEngine::UpdateDynamicObjects(renderer.Data(), entityManager.Data(), blitzenWorldContext);
            
            renderer->Update(drawContext);
            renderer->DrawFrame(drawContext);
        }

        // Reset window resize, TODO: Why is this here??????
        mainCamera.transformData.bWindowResize = false;

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