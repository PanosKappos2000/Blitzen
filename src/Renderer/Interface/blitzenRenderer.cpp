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

    bool RenderingResourcesInit(RenderingResources* pResources, RendererPtrType pRenderer)
    {
        if(!pRenderer->UploadTexture("Assets/Textures/base_baseColor.dds"))
		{
			BLIT_ERROR("Rendering resources failed");
			return false;
		}
        
        if (!pResources->m_textureManager.AddTexture(BlitzenCore::Ce_DefaultTextureName))
        {
            BLIT_ERROR("Something went wrong with texture map");
        }

        if (!pResources->m_textureManager.AddMaterial(0, 0, 0, 0, BlitzenCore::Ce_DefaultMaterialName))
        {
			BLIT_ERROR("Rendering resources failed");
            return false;
        }

        if (!LoadMeshFromObj(pResources->m_meshContext, "Assets/Meshes/bunny.obj", BlitzenCore::Ce_DefaultMeshName))
        {
			BLIT_ERROR("Rendering resources failed");
            return false;
        }

        // Success
        return true;
    }

    bool ManageGltf(const char* filepath, RenderingResources* pResources, BlitzenCore::EntityManager* pManager, RendererPtrType pRenderer)
    {
        auto& textureContext{ pResources->m_textureManager };
        auto& meshContext{ pResources->m_meshContext };
		auto& objectContext{ pManager->GetRenderContainer() };

        if (objectContext.m_renderCount >= BlitzenCore::Ce_MaxRenderObjects)
        {
            BLIT_WARN("BLITZEN_MAX_DRAW_OBJECT already reached, no more geometry can be loaded. GLTF LOADING FAILED!");
            return false;
        }

        CgltfScope cgltfScope{ nullptr };
        if(!LoadGltfFile(filepath, cgltfScope))
		{
			BLIT_ERROR("Failed to load GLTF file");
			return false;
		}

        // Texture count saved for materials
        auto previousTextureCount = textureContext.m_textureCount;

        // Textures (special care because they are directly managed by the renderer backend)
        BLIT_INFO("Loading textures for GLTF");
        for (size_t i = 0; i < cgltfScope.pData->textures_count; ++i)
        {
            // Change to dds texture
            auto pTexture = &cgltfScope.pData->textures[i];
            std::string ddsFilepath{ "" };
            if (!ModifyTextureFilepath(pTexture, filepath, ddsFilepath))
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

            // Type info thing kept here just because
            if (!pManager->template AddObject<BlitzenEngine::ClientTest>(renders, meshes, transform, true, "kitten"))
            {
                BLIT_ERROR("Failed to create dynamic object");
                return;
            }
        }
    }

    bool CreateSceneFromArguments(int argc, char** argv, BlitzenEngine::RenderingResources* pResources, BlitzenEngine::RendererPtrType pRenderer, BlitzenCore::EntityManager* pManager)
    {
        LoadTestGeometry(pResources->m_meshContext);
        //pResources->CreateSingleObjectForTesting();

        if constexpr (BlitzenCore::Ce_LoadDynamicObjectTest)
        {
            CreateDynamicObjectRendererTest(pManager->GetRenderContainer(), pResources->m_meshContext, pManager);
        }

        if (argc > 1)
        {
            // Special argument. Loads heavy scene to stress test the culling algorithms
            if (strcmp(argv[1], "RenderingStressTest") == 0)
            {
                LoadGeometryStressTest(pManager->GetRenderContainer(), pResources->m_meshContext, 3'000.f);

                // The following arguments are used as gltf filepaths
                for (int32_t i = 2; i < argc; ++i)
                {
                    if (!ManageGltf(argv[i], pResources, pManager, pRenderer))
                    {
                        BLIT_ERROR("Failed to load gltf scene from file: %s", argv[i]);
                        return false;
                    }
                }
            }

            else if (strcmp(argv[1], "InstancingStressTest") == 0)
            {
                LoadGeometryStressTest(pManager->GetRenderContainer(), pResources->m_meshContext, 2'000.f);

                // The following arguments are used as gltf filepaths
                for (int32_t i = 2; i < argc; ++i)
                {
                    if (!ManageGltf(argv[i], pResources, pManager, pRenderer))
                    {
                        BLIT_ERROR("Failed to load gltf scene from file: %s", argv[i]);
                        return false;
                    }
                }
            }

            // Special argument. Test oblique near-plane clipping technique. Not working yet.
            else if (strcmp(argv[1], "OnpcReflectionTest") == 0)
            {
                CreateObliqueNearPlaneClippingTestObject(pManager->GetRenderContainer(), pResources->m_meshContext);

                // The following arguments are used as gltf filepaths
                for (int32_t i = 2; i < argc; ++i)
                {
                    if (!ManageGltf(argv[i], pResources, pManager, pRenderer))
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
                    if (!ManageGltf(argv[i], pResources, pManager, pRenderer))
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