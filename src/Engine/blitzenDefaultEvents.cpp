#include "blitzenEngine.h"
#include "Renderer/blitRenderer.h"

namespace BlitzenEngine
{
    void RegisterDefaultEvents()
    {
        BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::EngineShutdown, nullptr, OnEvent);
        BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::KeyPressed, nullptr, OnKeyPress);
        BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::KeyReleased, nullptr, OnKeyPress);
        BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::WindowResize, nullptr, OnResize);
        BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::MouseMoved, nullptr, OnMouseMove);
    }

    uint8_t OnEvent(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data)
    {
        if(eventType == BlitzenCore::BlitEventType::EngineShutdown)
        {
            BLIT_WARN("Engine shutdown event encountered!")
            BlitzenEngine::Engine::GetEngineInstancePointer()->RequestShutdown();
            return 1; 
        }

        return 0;
    }

    uint8_t OnKeyPress(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data)
    {
        //Get the key pressed from the event context
        BlitzenCore::BlitKey key = static_cast<BlitzenCore::BlitKey>(data.data.ui16[0]);

        if(eventType == BlitzenCore::BlitEventType::KeyPressed)
        {
            switch(key)
            {
                case BlitzenCore::BlitKey::__ESCAPE:
                {
                    BlitzenCore::EventContext newContext = {};
                    BlitzenCore::FireEvent(BlitzenCore::BlitEventType::EngineShutdown, nullptr, newContext);
                    return 1;
                }
                // The four keys below control basic camera movement. They set a velocity and tell it that it should be updated based on that velocity
                case BlitzenCore::BlitKey::__W:
                {
                    Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
                    camera.transformData.cameraDirty = 1;
                    camera.transformData.velocity = BlitML::vec3(0.f, 0.f, 1.f);
                    break;
                }
                case BlitzenCore::BlitKey::__S:
                {
                    Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
                    camera.transformData.cameraDirty = 1;
                    camera.transformData.velocity = BlitML::vec3(0.f, 0.f, -1.f);
                    break;
                }
                case BlitzenCore::BlitKey::__A:
                {
                    Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
                    camera.transformData.cameraDirty = 1;
                    camera.transformData.velocity = BlitML::vec3(-1.f, 0.f, 0.f);
                    break;
                }
                case BlitzenCore::BlitKey::__D:
                {
                    Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
                    camera.transformData.cameraDirty = 1;
                    camera.transformData.velocity = BlitML::vec3(1.f, 0.f, 0.f);
                    break;
                }
                case BlitzenCore::BlitKey::__F1:
                {
                    Camera& cam = CameraSystem::GetCameraSystem()->GetCamera();

                    cam.transformData.freezeFrustum = !cam.transformData.freezeFrustum;
                }
                case BlitzenCore::BlitKey::__F2:
                {
                    ChangeDebugPyramidActiveState();
                    break;
                }
                case BlitzenCore::BlitKey::__F3:
                {
                    ChangeOcclusionCullingState();
                    break;
                }
                case BlitzenCore::BlitKey::__F4:
                {
                    ChangeLodEnabledState();
                    break;
                }
                case BlitzenCore::BlitKey::__F5:
                {
                    RenderingSystem::GetRenderingSystem()->SetActiveAPI(ActiveRenderer::Opengl);
                    break;
                }
                case BlitzenCore::BlitKey::__F6:
                {
                    RenderingSystem::GetRenderingSystem()->SetActiveAPI(ActiveRenderer::Vulkan);
                    break;
                }
                default:
                {
                    BLIT_DBLOG("Key pressed %i", key)
                    return 1;
                }
            }
        }
        else if (eventType == BlitzenCore::BlitEventType::KeyReleased)
        {
            switch (key)
            {
                case BlitzenCore::BlitKey::__W:
                case BlitzenCore::BlitKey::__S:
                {
                    Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
                    camera.transformData.velocity.z = 0.f;
                    if(camera.transformData.velocity.y == 0.f && camera.transformData.velocity.x == 0.f)
                    {
                        camera.transformData.cameraDirty = 0;
                    }
                    break;
                }
                case BlitzenCore::BlitKey::__A:
                case BlitzenCore::BlitKey::__D:
                {
                    Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
                    camera.transformData.velocity.x = 0.f;
                    if (camera.transformData.velocity.y == 0.f && camera.transformData.velocity.z == 0.f)
                    {
                        camera.transformData.cameraDirty = 0;
                    }
                    break;
                }
            }
        }
        return 0;
    }

    uint8_t OnResize(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data)
    {
        uint32_t newWidth = data.data.ui32[0];
        uint32_t newHeight = data.data.ui32[1];

        Engine::GetEngineInstancePointer()->UpdateWindowSize(newWidth, newHeight);

        return 1;
    }

    uint8_t OnMouseMove(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data)
    {
        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        float deltaTime = static_cast<float>(Engine::GetEngineInstancePointer()->GetDeltaTime());

        camera.transformData.cameraDirty = 1;

        RotateCamera(camera, deltaTime, data.data.si16[1], data.data.si16[0]);

        return 1;
    }
}