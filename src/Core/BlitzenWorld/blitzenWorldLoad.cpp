#include "blitzenWorldPrivate.h"
#include "BlitCL/blitclDebug.h"

namespace BlitzenWorld
{
    void LoadingLoop(int argc, char** argv, BlitzenPrivateContext& context, BlitzenEngine::DrawContext& drawContext)
    {
        while (true)
        {
            if (*context.pEngineState == BlitzenCore::EngineState::LOADING)
            {
                if (!CreateSceneFromArguments(argc, argv, context.pRenderingResources, context.pWORLD, context.pEntityMangager))
                {
                    BLIT_FATAL("Failed to allocate resource for requested scene, Blitzen shutting down");
                    
                    *context.pEngineState = BlitzenCore::EngineState::SHUTDOWN;
                    return;
                }

                if (!context.pWORLD->P_RENDERER->SetupForRendering(drawContext))
                {
                    BLIT_FATAL("Renderer failed to setup, Blitzen shutting down");
                    
                    *context.pEngineState = BlitzenCore::EngineState::SHUTDOWN;
                    return;
                }

                *context.pEngineState = BlitzenCore::EngineState::SETUP_AFTER_LOAD;

                BlitCL::LogContainerData(context.pWORLD->m_scenes, "Scene");

                return;
            }
        }
    }

    bool ManageGltf(const char* filepath, BlitzenEngine::RenderingResources* pResources, BlitzenEngine::EntityManager* pManager, BlitzenEngine::RendererPtrType pRenderer, BlitzenEngine::SceneContext* pScene)
    {
        auto& textureContext{ pResources->m_textureManager };
        auto& meshContext{ pResources->m_meshContext };
        auto& objectContext{ pManager->m_renderContainer };

        if (objectContext.m_renderCount >= BlitzenCore::Ce_MaxRenderObjects)
        {
            BLIT_WARN("BLITZEN_MAX_DRAW_OBJECT already reached, no more geometry can be loaded. GLTF LOADING FAILED!");
            return false;
        }

        if (pScene)
        {
            pScene->m_name.CopyString(filepath);
            int64_t truncationIndex = pScene->m_name.FindLastOf('.');

            if (truncationIndex == -1)
            {
                BLIT_INFO("Gltf name truncation failed");
            }
            else
            {
                pScene->m_name.Truncate(truncationIndex);
            }
        }

        BlitzenEngine::CgltfScope cgltfScope;
        cgltfScope.pData = nullptr;
        cgltfScope.m_pScene = pScene;

        if (!LoadGltfFile(filepath, cgltfScope))
        {
            BLIT_ERROR("Failed to load GLTF file");
            return false;
        }

        // Texture count saved for materials
        uint32_t previousTextureCount = textureContext.m_textureCount;

        // Textures (special care because they are directly managed by the renderer backend)
        BLIT_INFO("Loading textures for GLTF");
        for (size_t i = 0; i < cgltfScope.pData->textures_count; ++i)
        {
            // Change to dds texture
            auto pTexture = &cgltfScope.pData->textures[i];
            std::string ddsFilepath{ "" };
            if (!BlitzenEngine::ModifyTextureFilepath(pTexture, filepath, ddsFilepath))
            {
                BLIT_ERROR("Failed to get gltf texture filepath");
                return false;
            }

            // Give to renderer
            if (!pRenderer->UploadTexture(ddsFilepath.c_str()))
            {
                BLIT_ERROR("Renderer failed to create texture resource");
                return false;
            }

            // Update textures (might want to return if this fails)
            if (!textureContext.AddTexture(ddsFilepath.c_str()))
            {
                BLIT_ERROR("Texture system not updated properly for texture with path: %s", ddsFilepath.c_str());
            }
        }

        // Material count saved for meshes
        auto previousMaterialCount = textureContext.m_materialCount;

        // Materials
        BLIT_INFO("Loading materials for GLTF");
        LoadGltfMaterials(textureContext, cgltfScope, previousTextureCount);

        // Given to mesh loading to hold surface offsets for nodes
        BlitCL::DynamicArray<uint32_t> surfaceIndices(cgltfScope.pData->meshes_count);

        // Meshes
        BLIT_INFO("Loading meshes for GLTF");
        LoadGltfMeshes(meshContext, textureContext, cgltfScope, previousMaterialCount, surfaceIndices);

        BLIT_INFO("Loading scene nodes");
        LoadGltfNodes(objectContext, meshContext, cgltfScope, surfaceIndices);

        return true;
    }

    void LoadGeometryStressTest(BlitzenEngine::RenderContainer& renders, BlitzenEngine::MeshResources& meshContext, float transformMultiplier, BlitzenEngine::SceneContext* pScene)
    {
        constexpr uint32_t bunnyCount = 2'500'000;
        constexpr uint32_t kittenCount = 1'500'000;
        constexpr uint32_t maleCount = 90'000;
        constexpr uint32_t dragonCount = 10'000;
        constexpr uint32_t totalCount = bunnyCount + kittenCount + maleCount + dragonCount;

        BLIT_WARN("Loading Renderer Stress test with %i objects", totalCount);

        pScene->m_name.CopyString("RendererStressTestScene");

        for (uint32_t i = 0; i < BlitzenCore::Ce_EngineDefaultMeshesCount; ++i)
        {
            pScene->m_meshNames.EmplaceEmtpy();
        }

        pScene->m_meshNames[0].CopyString(BlitzenCore::Ce_DefaultMeshName);
        pScene->m_meshNames[1].CopyString(BlitzenCore::Ce_DefaultKittenMeshName);
        pScene->m_meshNames[2].CopyString(BlitzenCore::Ce_DefaultDragonMeshName);
        pScene->m_meshNames[3].CopyString(BlitzenCore::Ce_DefaultHumanMeshname);

        pScene->m_renderOffset = renders.m_renderCount;
        pScene->m_renderCount += bunnyCount + kittenCount + maleCount + dragonCount;

        uint32_t start = renders.m_renderCount;

        // Bunnies
        for (uint32_t i = start; i < start + bunnyCount; ++i)
        {
            CreateRenderObjectWithRandomTransform(meshContext.m_meshMap[BlitzenCore::Ce_DefaultMeshName].meshId, renders, meshContext, transformMultiplier, 5.f);
        }
        start += bunnyCount;

        // Kittens
        for (uint32_t i = start; i < start + kittenCount; ++i)
        {
            CreateRenderObjectWithRandomTransform(meshContext.m_meshMap[BlitzenCore::Ce_DefaultKittenMeshName].meshId, renders, meshContext, transformMultiplier, 1.f);
        }
        start += kittenCount;

        // Standford dragons
        for (uint32_t i = start; i < start + dragonCount; ++i)
        {
            CreateRenderObjectWithRandomTransform(meshContext.m_meshMap[BlitzenCore::Ce_DefaultDragonMeshName].meshId, renders, meshContext, transformMultiplier, 0.5f);
        }
        start += dragonCount;

        // Humans
        for (uint32_t i = start; i < start + maleCount; ++i)
        {
            CreateRenderObjectWithRandomTransform(meshContext.m_meshMap[BlitzenCore::Ce_DefaultHumanMeshname].meshId, renders, meshContext, transformMultiplier, 0.2f);
        }
    }

    struct WVRotatingKitten
    {
        BlitzenEngine::DynamicTransform* m_pTransform;
        uint32_t m_wvId{ BlitzenCore::Ce_MaxWorldVariableCount };

        inline void Start(WV_CREATE_CONTEXT& context)
        {
            m_pTransform = context.m_pDynamicTransform
        }
    };

    static void RotatingKittenPfn(void* data, float deltaTime)
    {
        auto pKitten{ reinterpret_cast<WVRotatingKitten*>(data) };

        BlitML::fRotation rotation{ 0.f, 0.1f, 0.f };

        BlitzenEngine::RotateEntity(pKitten->m_pTransform, rotation, BlitML::float3{ 0.f }, deltaTime, )
    }

    void CreateDynamicObjectRendererTest(BlitzenEngine::WV_CREATE_CONTEXT* worldVarArray, uint32_t contextCount, WORLD_blit* pWORLD)
    {
        pWORLD->m_worldVariableHost.ALLOC(worldVarArray, contextCount);

        for (uint32_t type = 0; type < contextCount; type++)
        {

        }
    }

    bool CreateSceneFromArguments(int argc, char** argv, BlitzenEngine::RenderingResources* pResources, WORLD_blit* pWORLD, BlitzenEngine::EntityManager* pManager)
    {
        LoadTestGeometry(pResources->m_meshContext);
        CreateSingleRender(pManager->m_renderContainer, pResources->m_meshContext, BlitzenCore::Ce_DefaultMeshName, 5.f);

        if constexpr (BlitzenCore::Ce_LoadDynamicObjectTest)
        {
            pWORLD->m_scenes.EmplaceEmtpy();
            CreateDynamicObjectRendererTest(pManager->m_renderContainer, pResources->m_meshContext, pManager, &pWORLD->m_scenes.Back());
        }

        if (argc > 1)
        {
            // Special argument. Loads heavy scene to stress test the culling algorithms
            if (strcmp(argv[1], "RenderingStressTest") == 0)
            {
                pWORLD->m_scenes.EmplaceEmtpy();
                LoadGeometryStressTest(pManager->m_renderContainer, pResources->m_meshContext, 3'000.f, &pWORLD->m_scenes.Back());

                // The following arguments are used as gltf filepaths
                for (int32_t i = 2; i < argc; ++i)
                {
                    pWORLD->m_scenes.EmplaceEmtpy();
                    if (!ManageGltf(argv[i], pResources, pManager, pWORLD->P_RENDERER.Data(), &pWORLD->m_scenes.Back()))
                    {
                        BLIT_ERROR("Failed to load gltf scene from file: %s", argv[i]);
                        return false;
                    }
                }
            }

            else if (strcmp(argv[1], "InstancingStressTest") == 0)
            {
                pWORLD->m_scenes.EmplaceEmtpy();
                LoadGeometryStressTest(pManager->m_renderContainer, pResources->m_meshContext, 2'000.f, &pWORLD->m_scenes.Back());

                // The following arguments are used as gltf filepaths
                for (int32_t i = 2; i < argc; ++i)
                {
                    pWORLD->m_scenes.EmplaceEmtpy();
                    if (!ManageGltf(argv[i], pResources, pManager, pWORLD->P_RENDERER.Data(), &pWORLD->m_scenes.Back()))
                    {
                        BLIT_ERROR("Failed to load gltf scene from file: %s", argv[i]);
                        return false;
                    }
                }
            }

            else if (strcmp(argv[1], "ClusterStressTest") == 0)
            {
                pWORLD->m_scenes.EmplaceEmtpy();
                LoadGeometryStressTest(pManager->m_renderContainer, pResources->m_meshContext, 1'500.f, &pWORLD->m_scenes.Back());

                // The following arguments are used as gltf filepaths
                for (int32_t i = 2; i < argc; ++i)
                {
                    pWORLD->m_scenes.EmplaceEmtpy();
                    if (!ManageGltf(argv[i], pResources, pManager, pWORLD->P_RENDERER.Data(), &pWORLD->m_scenes.Back()))
                    {
                        BLIT_ERROR("Failed to load gltf scene from file: %s", argv[i]);
                        return false;
                    }
                }
            }

            // Special argument. Test oblique near-plane clipping technique. Not working yet.
            else if (strcmp(argv[1], "OnpcReflectionTest") == 0)
            {
                pWORLD->m_scenes.EmplaceEmtpy();
                CreateObliqueNearPlaneClippingTestObject(pManager->m_renderContainer, pResources->m_meshContext);

                // The following arguments are used as gltf filepaths
                for (int32_t i = 2; i < argc; ++i)
                {
                    pWORLD->m_scenes.EmplaceEmtpy();
                    if (!ManageGltf(argv[i], pResources, pManager, pWORLD->P_RENDERER.Data(), &pWORLD->m_scenes.Back()))
                    {
                        BLIT_ERROR("Failed to load gltf scene from file: %s", argv[i]);
                        return false;
                    }
                }
            }
            else
            {
                pWORLD->m_scenes.EmplaceEmtpy();
                for (int32_t i = 1; i < argc; ++i)
                {
                    if (!ManageGltf(argv[i], pResources, pManager, pWORLD->P_RENDERER.Data(), &pWORLD->m_scenes.Back()))
                    {
                        BLIT_ERROR("Failed to load gltf scene from file: %s", argv[i]);
                        return false;
                    }
                }
            }
        }

        return true;
    }
}