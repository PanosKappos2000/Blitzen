#define CGLTF_IMPLEMENTATION
#include "blitRenderer.h"

namespace BlitzenEngine
{
    void UpdateRendererTransform(RendererPtrType pRenderer, RendererTransformUpdateContext& context)
    {
        pRenderer->UpdateObjectTransform(context);
    }

    void UpdateDynamicObjects(RendererPtrType pRenderer, BlitzenCore::EntityManager* pEntityManager, BlitzenWorld::BlitzenWorldContext& blitzenContext)
    {
		auto& pDynamics{ pEntityManager->m_pDynamicObjects };
        for (auto pObject : pDynamics)
        {
            pObject->Update(blitzenContext);

            switch (blitzenContext.rendererEvent)
            {
            case BlitzenEngine::RendererEvent::RENDERER_TRANSFORM_UPDATE:
            {
                UpdateRendererTransform(pRenderer, blitzenContext.rendererTransformUpdate);
                break;
            }
            case BlitzenEngine::RendererEvent::MAX_RENDERER_EVENTS:
            default:
            {
                break;
            }
            }
        }
    }

    bool LoadTextureFromFile(BlitzenEngine::RenderingResources* pResources, const char* filename, const char* texName, RendererPtrType pRenderer)
    {
		auto textureCount = pResources->GetTextureCount();
		auto textures{ pResources->GetTextureArrayPointer() };

        // Don't go over the texture limit, might want to throw a warning here
        if (textureCount >= BlitzenCore::Ce_MaxTextureCount)
        {
            BLIT_WARN("Max texture count: %i, has been reached", BlitzenCore::Ce_MaxTextureCount);
            BLIT_ERROR("Texture from file: %s, failed to load", filename);
            return 0;
        }

        auto& texture = textures[textureCount];
        // If texture upload to the renderer succeeds, the texture count is incremented and the function returns successfully
        if (pRenderer->UploadTexture(texture.pTextureData, filename))
        {
            pResources->IncrementTextureCount();
            return true;
        }
        else
        {
            BLIT_ERROR("Texture from file: %s, failed to load", filename);
            return false;
        }
    }

    void LoadGltfTextures(BlitzenEngine::RenderingResources* pResources, const cgltf_data* pGltfData, uint32_t previousTextureCount, const char* gltfFilepath, RendererPtrType pRenderer)
    {
        for (size_t i = 0; i < pGltfData->textures_count; ++i)
        {
            auto pTexture = &pGltfData->textures[i];
            if (!pTexture->image)
            {
                break;
            }

            auto pImage = pTexture->image;
            if (!pImage->uri)
            {
                break;
            }

            std::string ipath = gltfFilepath;
            auto pos = ipath.find_last_of('/');
            if (pos == std::string::npos)
            {
                ipath = "";
            }
            else
            {
                ipath = ipath.substr(0, pos + 1);
            }

            std::string uri{ pImage->uri };
            uri.resize(cgltf_decode_uri(&uri[0]));
            auto dot = uri.find_last_of('.');

            if (dot != std::string::npos)
            {
                uri.replace(dot, uri.size() - dot, ".dds");
            }

            auto path = ipath + uri;

            LoadTextureFromFile(pResources, path.c_str(), path.c_str(), pRenderer);
        }
    }

    bool LoadGltfScene(RenderingResources* pResources, RenderContainer& renders, const char* path, RendererPtrType pRenderer)
    {
        if (renders.m_renderCount >= BlitzenCore::Ce_MaxRenderObjects)
        {
            BLIT_WARN("BLITZEN_MAX_DRAW_OBJECT already reached, no more geometry can be loaded. GLTF LOADING FAILED!");
            return false;
        }

        cgltf_options options{};

        cgltf_data* pData{ nullptr };

        auto res = cgltf_parse_file(&options, path, &pData);
        if (res != cgltf_result_success)
        {
            BLIT_WARN("Failed to load gltf file: %s", path)
                return false;
        }

        // Automatic free struct
        struct CgltfScope
        {
            cgltf_data* pData;
            inline ~CgltfScope() { cgltf_free(pData); }
        };
        CgltfScope cgltfScope{ pData };

        res = cgltf_load_buffers(&options, pData, path);
        if (res != cgltf_result_success)
        {
            return false;
        }


        res = cgltf_validate(pData);
        if (res != cgltf_result_success)
        {
            return false;
        }

        BLIT_INFO("Loading GLTF scene from file: %s", path);

        // Textures
        auto previousTextureCount = pResources->GetTextureCount();
        BLIT_INFO("Loading textures");
        LoadGltfTextures(pResources, pData, previousTextureCount, path, pRenderer);

        // Materials
        BLIT_INFO("Loading materials");
        auto previousMaterialCount = pResources->GetMaterialCount();
        LoadGltfMaterials(pResources, pData, previousMaterialCount, previousTextureCount);

        // Meshes and primitives
        BlitCL::DynamicArray<uint32_t> surfaceIndices(pData->meshes_count);
        auto& meshContext{ pResources->GetMeshContext() };
        auto* pMaterials{ const_cast<Material*>(pResources->GetMaterialArrayPointer()) };
        LoadGltfMeshes(meshContext, pMaterials, pData, path, previousMaterialCount, surfaceIndices);

        // Render objects
        BLIT_INFO("Loading scene nodes");
        LoadGltfNodes(renders, pResources->GetMeshContext(), pData, surfaceIndices);

        return true;
    }

    void CreateDynamicObjectRendererTest(BlitzenEngine::RenderContainer& renders, BlitzenEngine::MeshResources& meshes, BlitzenCore::EntityManager* pManager)
    {
        const uint32_t ObjectCount = BlitzenCore::Ce_MaxDynamicObjectCount;
        if (pManager->GetRenderCount() + ObjectCount > BlitzenCore::Ce_MaxRenderObjects)
        {
            BLIT_ERROR("Could not add dynamic object renderer test, object count exceeds limit");
            return;
        }

        for (size_t i = 0; i < ObjectCount; ++i)
        {
            BlitzenEngine::MeshTransform transform;
            RandomizeTransform(transform, 100.f, 1.f);

            // The lambda method is a different way of having mutliple types of dynamic object with different update logic
            // The normal version is more elegant(and has actual sanity), but this one is pretty cool still.
#if defined(LAMBDA_GAME_OBJECT_TEST)
            struct ClientTestDataStruct
            {
                float yaw;
                float pitch;
                float speed;
            };
            ClientTestDataStruct objectData{ 0.f, 0.f, 0.1f };

            auto createResult{ !pManager->AddObject<BlitzenEngine::ClientTest>(pResources, transform,
            {
                [](GameObject* pObject)
                {
                    auto pData = pObject->m_pData.Get<ClientTestDataStruct>();
                    RotateObject(pData->yaw, pData->pitch, pObject->m_transformId, pData->speed);
                }
            }, true, "kitten", &objectData, sizeof(ClientTestDataStruct)) };

            if (!createResult)
            {
                BLIT_ERROR("Failed to create dynamic object");
                return;
            }
#else
// Normally I should call this outside the if statement and store the result to avoid the use of template keyword in the dependend scope
// But this is very interesting and I have not used that keyword before so I am leaving it.
            if (!pManager->template AddObject<BlitzenEngine::ClientTest>(renders, meshes, transform, true, "kitten"))
            {
                BLIT_ERROR("Failed to create dynamic object");
                return;
            }
#endif
        }
    }

    bool CreateSceneFromArguments(int argc, char** argv, BlitzenEngine::RenderingResources* pResources, BlitzenEngine::RendererPtrType pRenderer, BlitzenCore::EntityManager* pManager)
    {
        LoadTestGeometry(pResources->GetMeshContext());
        //pResources->CreateSingleObjectForTesting();

        if constexpr (BlitzenCore::Ce_LoadDynamicObjectTest)
        {
            CreateDynamicObjectRendererTest(pManager->GetRenderContainer(), pResources->GetMeshContext(), pManager);
        }

        if (argc > 1)
        {
            // Special argument. Loads heavy scene to stress test the culling algorithms
            if (strcmp(argv[1], "RenderingStressTest") == 0)
            {
                LoadGeometryStressTest(pManager->GetRenderContainer(), pResources->GetMeshContext(), 3'000.f);

                // The following arguments are used as gltf filepaths
                for (int32_t i = 2; i < argc; ++i)
                {
                    if (!LoadGltfScene(pResources, pManager->GetRenderContainer(), argv[i], pRenderer))
                    {
                        BLIT_ERROR("Failed to load gltf scene from file: %s", argv[i]);
                        return false;
                    }
                }
            }

            else if (strcmp(argv[1], "InstancingStressTest") == 0)
            {
                LoadGeometryStressTest(pManager->GetRenderContainer(), pResources->GetMeshContext(), 2'000.f);

                // The following arguments are used as gltf filepaths
                for (int32_t i = 2; i < argc; ++i)
                {
                    if (!LoadGltfScene(pResources, pManager->GetRenderContainer(), argv[i], pRenderer))
                    {
                        BLIT_ERROR("Failed to load gltf scene from file: %s", argv[i]);
                        return false;
                    }
                }
            }

            // Special argument. Test oblique near-plane clipping technique. Not working yet.
            else if (strcmp(argv[1], "OnpcReflectionTest") == 0)
            {
                CreateObliqueNearPlaneClippingTestObject(pManager->GetRenderContainer(), pResources->GetMeshContext());

                // The following arguments are used as gltf filepaths
                for (int32_t i = 2; i < argc; ++i)
                {
                    if (!LoadGltfScene(pResources, pManager->GetRenderContainer(), argv[i], pRenderer))
                    {
                        BLIT_ERROR("Failed to load gltf scene from file: %s", argv[i]);
                        return false;
                    }
                }
            }
            else
            {
                for (int32_t i = 1; i < argc; ++i)
                {
                    if (!LoadGltfScene(pResources, pManager->GetRenderContainer(), argv[i], pRenderer))
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