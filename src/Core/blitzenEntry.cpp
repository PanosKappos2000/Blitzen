#include "Core/Events/blitEvents.h"
#include "Platform/blitPlatformContext.h"
#include "Platform/blitPlatform.h"

using EventSystemMemory = BlitCL::SmartPointer<BlitzenCore::EventSystem>;
using RndResourcesMemory = BlitCL::SmartPointer<BlitzenEngine::RenderingResources, BlitzenCore::AllocationType::Renderer>;


#if defined(BLIT_GDEV_EDT)
int main(int argc, char* argv[])
{
    /* ENGINE SYSTEMS INITIALIZATION */
    BlitzenCore::Engine blitzenEngine;
    blitzenEngine.m_state = BlitzenCore::EngineState::LOADING;

    BlitzenWorld::BlitzenPrivateContext blitzenPrivateContext{};
    BlitzenWorld::BlitzenWorldContext blitzenWorldContext{};
	blitzenPrivateContext.pBlitzenContext = &blitzenWorldContext;

    BlitzenCore::InitLogging();

    blitzenPrivateContext.pEngineState = &blitzenEngine.m_state;

    BlitzenEngine::CameraContainer blitzenCameraSystem;
    auto& mainCamera = blitzenCameraSystem.GetMainCamera();
    BlitzenEngine::SetupCamera(mainCamera);
    blitzenWorldContext.pCameraContainer = &blitzenCameraSystem;

    BlitzenCore::WorldTimeManager blitzenClock;
    blitzenWorldContext.pCoreClock = &blitzenClock;

    BlitzenEngine::EntitySystemMemory blitzenEntityManager;
    blitzenEntityManager.Make();
    blitzenPrivateContext.pEntityMangager = blitzenEntityManager.Data();

    RndResourcesMemory renderingResources;
    renderingResources.Make();
    blitzenPrivateContext.pRenderingResources = renderingResources.Data();

    BlitzenPlatform::PlatformContext platform{};
    blitzenPrivateContext.pPlatform = &platform;

    BlitzenEngine::WORLD_blit WORLD{ mainCamera, renderingResources->m_meshContext, blitzenEntityManager->m_renderContainer, renderingResources->m_textureManager, &platform };
    WORLD.P_RENDERER.Make();
    blitzenPrivateContext.pWORLD = &WORLD;
    
    EventSystemMemory blitzenEventSystem;
    blitzenEventSystem.Make(std::ref(blitzenWorldContext), std::ref(blitzenPrivateContext));

    BlitzenCore::Dasher dasher;
    blitzenPrivateContext.pDasher = &dasher;

    BlitzenPlatform::PlatformArgs platformArgs{&platform, blitzenEventSystem.Data(), WORLD.P_RENDERER.Data(), &dasher};
    BLIT_ASSERT(BlitzenPlatform::SystemStartup(platformArgs));

    BLIT_ASSERT(RenderingResourcesInit(renderingResources.Data(), WORLD.P_RENDERER.Data()));

    // LOADING RESOURCES
    std::thread loadingThread
    {
        [&]()
        {
            BlitzenWorld::LoadingLoop(argc, argv, blitzenPrivateContext, WORLD.m_drawContext);
        }
    };
    #if(_WIN32)
        loadingThread.detach();
    #else
        loadingThread.join();
    #endif

    // LOOP
    while(blitzenEngine.m_state != BlitzenCore::EngineState::SHUTDOWN)
    {
        if (!BlitzenPlatform::DispatchEvents(&platform))
        {
            blitzenEngine.m_state = BlitzenCore::EngineState::SHUTDOWN;
        }

        switch (blitzenEngine.m_state)
        {
        case BlitzenCore::EngineState::RUNNING:
        {
            BlitzenCore::UpdateWorldClock(blitzenClock);

            BlitzenEngine::UpdateCamera(mainCamera, float(blitzenClock.m_deltaTime));

            BlitzenEngine::UpdateEntityComponents(WORLD.P_RENDERER.Data(), blitzenEntityManager.Data(), float(blitzenClock.m_deltaTime));

            WORLD.P_RENDERER.Data()->DrawFrame(WORLD.m_drawContext);

#if defined(DASHER_JOIN)
            dasher.Draw((float)blitzenClock.m_deltaTime);
#endif
            WORLD.P_RENDERER.Data()->Present();

            break;
        }
        case BlitzenCore::EngineState::RUNNING_EDITOR_NO_START:
        {
            BlitzenCore::UpdateWorldClock(blitzenClock);

            BlitzenEngine::UpdateCamera(mainCamera, float(blitzenClock.m_deltaTime));

            BlitzenEngine::UpdateEntityComponents(WORLD.P_RENDERER.Data(), blitzenEntityManager.Data(), float(blitzenClock.m_deltaTime));

            WORLD.P_RENDERER.Data()->DrawFrame(WORLD.m_drawContext);

            WORLD.P_RENDERER.Data()->Present(1);

            break;
        }
        case BlitzenCore::EngineState::LOADING:
        {
            BlitzenCore::UpdateWorldClock(blitzenClock);

            WORLD.P_RENDERER.Data()->DrawWhileWaiting(float(blitzenClock.m_deltaTime));

            break;
        }
        case BlitzenCore::EngineState::SETUP_AFTER_LOAD:
        {
            WORLD.P_RENDERER.Data()->FinalSetup();

            blitzenEngine.m_state = BlitzenCore::EngineState::RUNNING;

            break;
        }
        case BlitzenCore::EngineState::SUSPENDED:
        {
            break;
        }
        case BlitzenCore::EngineState::MAX_STATES:
        default:
        {
            blitzenEngine.m_state = BlitzenCore::EngineState::SHUTDOWN;
            break;
        }
        }

#if defined(DASHER_JOIN) && defined(DASHER_USE_DEAR)
        // Using IMGUI for the editor requires some extra care for event handling
        blitzenEventSystem->UpdateInput(blitzenClock.m_deltaTime, &dasher.m_eventContext);
#else
        blitzenEventSystem->UpdateInput(blitzenClock.m_deltaTime);
#endif
    }
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