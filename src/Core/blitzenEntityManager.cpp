#include "blitEntityManager.h"

namespace BlitzenCore
{   
    void GameObjectManager::UpdateDynamicObjects(BlitzenWorld::BlitzenWorldContext& blitzenContext, BlitzenWorld::BlitzenPrivateContext& privateContext)
    {
        for (auto pObject : m_pDynamicObjects)
        {
            blitzenContext.rendererEvent = BlitzenEngine::RendererEvent::MAX_RENDERER_EVENTS;

            pObject->Update(blitzenContext);

            switch (blitzenContext.rendererEvent)
            {
            case BlitzenEngine::RendererEvent::RENDERER_TRANSFORM_UPDATE:
            {
                UpdateRendererTransform(privateContext.pRenderer, blitzenContext.rendererTransformUpdate);
                break;
            }
            case BlitzenEngine::RendererEvent::MAX_RENDERER_EVENTS:
            {
                break;
            }
            }

        }
    }

    void CreateDynamicObjectRendererTest(BlitzenEngine::RenderingResources* pResources, GameObjectManager* pManager)
    {
        const uint32_t ObjectCount = BlitzenCore::Ce_MaxDynamicObjectCount;
        if (pResources->renderObjectCount + ObjectCount > BlitzenCore::Ce_MaxRenderObjects)
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
            if (!pManager->template AddObject<BlitzenEngine::ClientTest>(pResources, transform, true, "kitten"))
            {
                BLIT_ERROR("Failed to create dynamic object");
                return;
            }
            #endif
        }
    }

    bool CreateSceneFromArguments(int argc, char** argv, BlitzenEngine::RenderingResources* pResources, BlitzenEngine::RendererPtrType pRenderer, GameObjectManager* pManager)
    {
        LoadTestGeometry(pResources);
        //pResources->CreateSingleObjectForTesting();

        #if defined(BLIT_DYNAMIC_OBJECT_TEST)
            CreateDynamicObjectRendererTest(pResources, pManager);
        #endif

        if (argc > 1)
        {
            // Special argument. Loads heavy scene to stress test the culling algorithms
            if (strcmp(argv[1], "RenderingStressTest") == 0)
            {
                LoadGeometryStressTest(pResources, 3'000.f);

                // The following arguments are used as gltf filepaths
                for (int32_t i = 2; i < argc; ++i)
                {
                    if (!BlitzenEngine::LoadGltfScene(pResources, argv[i], pRenderer))
                    {
                        BLIT_ERROR("Failed to load gltf scene from file: %s", argv[i]);
                        return false;
                    }
                }
            }

            else if (strcmp(argv[1], "InstancingStressTest") == 0)
            {
                LoadGeometryStressTest(pResources, 2'000.f);

                // The following arguments are used as gltf filepaths
                for (int32_t i = 2; i < argc; ++i)
                {
                    if (!BlitzenEngine::LoadGltfScene(pResources, argv[i], pRenderer))
                    {
                        BLIT_ERROR("Failed to load gltf scene from file: %s", argv[i]);
                        return false;
                    }
                }
            }

            // Special argument. Test oblique near-plane clipping technique. Not working yet.
            else if (strcmp(argv[1], "OnpcReflectionTest") == 0)
            {
                CreateObliqueNearPlaneClippingTestObject(pResources);

                // The following arguments are used as gltf filepaths
                for (int32_t i = 2; i < argc; ++i)
                {
                    if (!BlitzenEngine::LoadGltfScene(pResources, argv[i], pRenderer))
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
                    if (!BlitzenEngine::LoadGltfScene(pResources, argv[i], pRenderer))
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