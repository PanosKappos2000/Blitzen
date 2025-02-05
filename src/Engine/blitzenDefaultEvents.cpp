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
                    Camera* pCamera = CameraSystem::GetCameraSystem()->GetMovingCamera();
                    pCamera->cameraDirty = 1;
                    pCamera->velocity = BlitML::vec3(0.f, 0.f, 1.f);
                    break;
                }
                case BlitzenCore::BlitKey::__S:
                {
                    Camera* pCamera = CameraSystem::GetCameraSystem()->GetMovingCamera();
                    pCamera->cameraDirty = 1;
                    pCamera->velocity = BlitML::vec3(0.f, 0.f, -1.f);
                    break;
                }
                case BlitzenCore::BlitKey::__A:
                {
                    Camera* pCamera = CameraSystem::GetCameraSystem()->GetMovingCamera();
                    pCamera->cameraDirty = 1;
                    pCamera->velocity = BlitML::vec3(-1.f, 0.f, 0.f);
                    break;
                }
                case BlitzenCore::BlitKey::__D:
                {
                    Camera* pCamera = CameraSystem::GetCameraSystem()->GetMovingCamera();
                    pCamera->cameraDirty = 1;
                    pCamera->velocity = BlitML::vec3(1.f, 0.f, 0.f);
                    break;
                }
                case BlitzenCore::BlitKey::__F1:
                {
                    CameraSystem* pSystem = CameraSystem::GetCameraSystem()->GetCameraSystem();
                    Camera& main = pSystem->GetCamera();
                    Camera* cameraList = pSystem->GetCameraList();

                    // If the moving camera not detatched from the main, detatches it
                    if(pSystem->GetMovingCamera() == &(cameraList[BLIT_MAIN_CAMERA_ID]))
                    {
                        Camera& detatched = cameraList[BLIT_DETATCHED_CAMERA_ID];
                        SetupCamera(detatched, main.fov, main.windowWidth, main.windowHeight, main.zNear, main.position, 
                        main.yawRotation, main.pitchRotation);
                        CameraSystem::GetCameraSystem()->SetMovingCamera(&detatched);
                    }
                    // Otherwise, does the opposite
                    else
                    {
                        CameraSystem::GetCameraSystem()->SetMovingCamera(&main);
                        // Tell the renderer to change the camera without waiting for movement
                        main.cameraDirty = 1;// This might break some logic in some cases, but it can easily be fixed
                    }
                    break;
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
                    SetActiveRenderer(ActiveRenderer::Opengl);
                    break;
                }
                case BlitzenCore::BlitKey::__F6:
                {
                    SetActiveRenderer(ActiveRenderer::Vulkan);
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
                    Camera* pCamera = CameraSystem::GetCameraSystem()->GetMovingCamera();
                    pCamera->velocity.z = 0.f;
                    if(pCamera->velocity.y == 0.f && pCamera->velocity.x == 0.f)
                    {
                        pCamera->cameraDirty = 0;
                    }
                    break;
                }
                case BlitzenCore::BlitKey::__A:
                case BlitzenCore::BlitKey::__D:
                {
                    Camera* pCamera = CameraSystem::GetCameraSystem()->GetMovingCamera();
                    pCamera->velocity.x = 0.f;
                    if (pCamera->velocity.y == 0.f && pCamera->velocity.z == 0.f)
                    {
                        pCamera->cameraDirty = 0;
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
        Camera& camera = *(CameraSystem::GetCameraSystem()->GetMovingCamera());
        float deltaTime = static_cast<float>(Engine::GetEngineInstancePointer()->GetDeltaTime());

        camera.cameraDirty = 1;

        RotateCamera(camera, deltaTime, data.data.si16[1], data.data.si16[0]);

        return 1;
    }
}