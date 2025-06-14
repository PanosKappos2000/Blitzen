#include "Core/Events/blitEvents.h"
#include "Platform/blitPlatformContext.h"
#include "Platform/blitPlatform.h"


using RndResourcesMemory = BlitCL::SmartPointer<BlitzenEngine::RenderingResources, BlitzenCore::AllocationType::Renderer>;


#if defined(BLIT_GDEV_EDT)
int main(int argc, char* argv[])
{
    /* ENGINE SYSTEMS INITIALIZATION */
    BlitzenCore::Engine blitzenEngine;
    blitzenEngine.m_state = BlitzenCore::EngineState::LOADING;

    BlitzenWorld::BlitzenPrivateContext blitzenPrivateContext{};

    BlitzenCore::InitLogging();

    blitzenPrivateContext.pEngineState = &blitzenEngine.m_state;

    BlitzenEngine::CameraContainer blitzenCameraSystem;
    auto& mainCamera = blitzenCameraSystem.GetMainCamera();
    BlitzenEngine::SetupCamera(mainCamera);

    BlitzenCore::WorldTimeManager blitzenClock;
    blitzenPrivateContext.pClock = &blitzenClock;

    BlitzenEngine::ComponentSystemMemory blitzenComponentSystem;
    blitzenComponentSystem.Make();
    blitzenPrivateContext.pComponents = blitzenComponentSystem.Data();

    RndResourcesMemory renderingResources;
    renderingResources.Make();
    blitzenPrivateContext.pRenderingResources = renderingResources.Data();

    BlitzenPlatform::PlatformContext platform{};
    blitzenPrivateContext.pPlatform = &platform;

    BlitzenWorld::WORLD_blit WORLD{ mainCamera, renderingResources->m_meshContext, renderingResources->m_textureManager, &platform };
    WORLD.pCameraContainer = &blitzenCameraSystem;
    WORLD.P_RESIDENTS.Make();
    WORLD.m_drawContext.m_pResidents = WORLD.P_RESIDENTS.Data();
    WORLD.P_RENDERER.Make();
    blitzenPrivateContext.pWORLD = &WORLD;
    
    BlitzenCore::EventSystemMemory blitzenEventSystem;
    blitzenEventSystem.Make(std::ref(WORLD), std::ref(blitzenPrivateContext));

    BlitzenCore::Dasher dasher;
    blitzenPrivateContext.pDasher = &dasher;

    BlitzenPlatform::PlatformArgs platformArgs{&platform, blitzenEventSystem.Data(), WORLD.P_RENDERER.Data(), &dasher};
    BLIT_ASSERT(BlitzenPlatform::SystemStartup(platformArgs));

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
        BlitzenWorld::WorldLoop(blitzenPrivateContext);

        BlitzenWorld::UpdateLoop(blitzenPrivateContext);

        BlitzenWorld::RenderLoop(blitzenPrivateContext);

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