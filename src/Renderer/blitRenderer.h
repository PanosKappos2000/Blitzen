#pragma once
// Vulkan
#include "BlitzenVulkan/vulkanRenderer.h"
// Opengl
#if defined(_WIN32)
#include "BlitzenGl/openglRenderer.h"
#endif
// Direct3D12
#if defined(_WIN32)
#include "BlitzenDX12/dx12Renderer.h"
#endif
#include <typeinfo>

#include <cstring> // For strcmp

namespace BlitzenEngine
{
    #if defined(BLIT_GL_LEGACY_OVERRIDE) && !defined(BLIT_VK_FORCE) && defined(_WIN32)

        using Renderer = BlitCL::SmartPointer<BlitzenGL::OpenglRenderer, BlitzenCore::AllocationType::Renderer>;

        using RendererPtrType = BlitzenGL::OpenglRenderer*;

		using RendererType = BlitzenGL::OpenglRenderer;

    #elif defined(BLIT_VK_FORCE) || defined(linux)

        using Renderer = BlitCL::SmartPointer<BlitzenVulkan::VulkanRenderer, BlitzenCore::AllocationType::Renderer>;

        using RendererPtrType = BlitzenVulkan::VulkanRenderer*;

        using RendererType = BlitzenVulkan::VulkanRenderer;

    #else

        using Renderer = BlitCL::SmartPointer<BlitzenDX12::Dx12Renderer, BlitzenCore::AllocationType::Renderer>;

        using RendererPtrType = BlitzenDX12::Dx12Renderer*;

        using RendererType = BlitzenDX12::Dx12Renderer;

    #endif


    /*
        Renderer testing functions
    */
    template<typename MT>
    void CreateDynamicObjectRendererTest(RenderingResources* pResources, MT* pManager)
    {
        constexpr uint32_t ce_objectCount = Ce_MaxDynamicObjectCount;
        if (pResources->renderObjectCount + ce_objectCount > ce_maxRenderObjects)
        {
            BLIT_ERROR("Could not add dynamic object renderer test, object count exceeds limit");
            return;
        }

        for (size_t i = 0; i < ce_objectCount; ++i)
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
            if (!pManager->AddObject<BlitzenEngine::ClientTest>(pResources, transform,
                { [](GameObject* pObject)
                {
                    auto pData = pObject->m_pData.Get<ClientTestDataStruct>();
                    RotateObject(pData->yaw, pData->pitch,
                        pObject->m_transformId, pData->speed);
                } },
                true, "kitten", &objectData, sizeof(ClientTestDataStruct)))
            {
                return;
            }
            #else
            // Normally I should call this outside the if statement and store the result to avoid the use of template keyword in the dependend scope
            // But this is very interesting and I have not used that keyword before so I am leaving it.
            if (!pManager->template AddObject<BlitzenEngine::ClientTest>(pResources, transform, true, "kitten"))
            {
                return;
            }
            #endif
        }
    }

    // Takes the command line arguments to form a scene (this is pretty ill formed honestly)
    template<class RT, class MT>
    void CreateSceneFromArguments(int argc, char** argv,
        RenderingResources* pResources, RT* pRenderer, MT* pManager)
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
                LoadGeometryStressTest(pResources);

                // The following arguments are used as gltf filepaths
                for (int32_t i = 2; i < argc; ++i)
                {
                    pResources->LoadGltfScene(argv[i], pRenderer);
                }
            }

            // Special argument. Test oblique near-plane clipping technique. Not working yet.
            else if (strcmp(argv[1], "OnpcReflectionTest") == 0)
            {
                CreateObliqueNearPlaneClippingTestObject(pResources);

                // The following arguments are used as gltf filepaths
                for (int32_t i = 2; i < argc; ++i)
                {
                    pResources->LoadGltfScene(argv[i], pRenderer);
                }
            }

            // If there are no special arguments everything is treated as a filepath for a gltf scene
            else
            {
                for (int32_t i = 1; i < argc; ++i)
                {
                    pResources->LoadGltfScene(argv[i], pRenderer);
                }
            }
        }
    }
}