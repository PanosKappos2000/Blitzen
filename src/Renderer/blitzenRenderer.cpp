#include "Renderer/blitRenderer.h"

namespace BlitzenEngine
{
#if defined(_WIN32)
    void UpdateRendererTransform(BlitzenDX12::Dx12Renderer* pDx12, RendererTransformUpdateContext& context)
    {
		pDx12->UpdateObjectTransform(context);
    }

    void UpdateRendererTransform(BlitzenGL::OpenglRenderer* pGL, RendererTransformUpdateContext& context)
    {
		pGL->UpdateObjectTransform(context);
    }
#endif
    void UpdateRendererTransform(BlitzenVulkan::VulkanRenderer* pVK, RendererTransformUpdateContext& context)
    {
		pVK->UpdateObjectTransform(context);
    }

    void CreateDynamicObjectRendererTest(RenderingResources* pResources, BlitzenEngine::GameObjectManager* pManager)
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
            if (!pManager->AddObject<BlitzenEngine::ClientTest>(pResources, transform,
                {
                    [](GameObject* pObject)
                    {
                        auto pData = pObject->m_pData.Get<ClientTestDataStruct>();
                        RotateObject(pData->yaw, pData->pitch, pObject->m_transformId, pData->speed);
                    }
                }, true, "kitten", &objectData, sizeof(ClientTestDataStruct)))
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
}