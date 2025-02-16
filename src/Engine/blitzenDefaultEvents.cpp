#include "blitzenEngine.h"
#include "Renderer/blitRenderer.h"

namespace BlitzenEngine
{
    uint8_t OnEvent(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, 
    const BlitzenCore::EventContext& data)
    {
        if(eventType == BlitzenCore::BlitEventType::EngineShutdown)
        {
            BLIT_WARN("Engine shutdown event encountered!")
            BlitzenEngine::Engine::GetEngineInstancePointer()->RequestShutdown();
            return 1; 
        }

        return 0;
    }

    void CloseOnEscapeKeyPressCallback()
    {
        BlitzenCore::FireEvent(BlitzenCore::BlitEventType::EngineShutdown, nullptr, {});
    }

    void MoveDefaultCameraForwardOnWKeyPressCallback()
    {
        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        camera.transformData.cameraDirty = 1;
        camera.transformData.velocity = BlitML::vec3(0.f, 0.f, 1.f);
    }

    void StopMovingCameraForwardOnWKeyReleaseCallback()
    {
        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        camera.transformData.velocity.z = 0.f;
        if(camera.transformData.velocity.y == 0.f && camera.transformData.velocity.x == 0.f)
        {
            camera.transformData.cameraDirty = 0;
        }
    }

    void MoveDefaultCameraBackwardOnSKeyPressCallback()
    {
        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        camera.transformData.cameraDirty = 1;
        camera.transformData.velocity = BlitML::vec3(0.f, 0.f, -1.f);
    }

    void StopMovingCameraBackwardOnSKeyReleaseCallback()
    {
        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        camera.transformData.velocity.z = 0.f;
        if(camera.transformData.velocity.y == 0.f && camera.transformData.velocity.x == 0.f)
        {
            camera.transformData.cameraDirty = 0;
        }
    }

    void MoveDefaultCameraLeftOnAKeyPressCallback()
    {
        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        camera.transformData.cameraDirty = 1;
        camera.transformData.velocity = BlitML::vec3(-1.f, 0.f, 0.f);
    }

    void StopMovingCameraLeftOnAKeyReleaseCallback()
    {
        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        camera.transformData.velocity.x = 0.f;
        if (camera.transformData.velocity.y == 0.f && camera.transformData.velocity.z == 0.f)
        {
            camera.transformData.cameraDirty = 0;
        }
    }

    void MoveDefaultCameraRightOnDKeyPressCallback()
    {
        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        camera.transformData.cameraDirty = 1;
        camera.transformData.velocity = BlitML::vec3(1.f, 0.f, 0.f);
    }

    void StopMovingCameraRightOnDReleaseCallback()
    {
        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        camera.transformData.velocity.x = 0.f;
        if (camera.transformData.velocity.y == 0.f && camera.transformData.velocity.z == 0.f)
        {
            camera.transformData.cameraDirty = 0;
        }
    }

    void FreezeFrustumOnF1KeyPressCallback()
    {
        Camera& cam = CameraSystem::GetCameraSystem()->GetCamera();
        cam.transformData.freezeFrustum = !cam.transformData.freezeFrustum;
    }

    uint8_t OnKeyPress(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, 
    const BlitzenCore::EventContext& data)
    {
        //Get the key pressed from the event context
        BlitzenCore::BlitKey key = static_cast<BlitzenCore::BlitKey>(data.data.ui16[0]);

        if(eventType == BlitzenCore::BlitEventType::KeyPressed)
            BlitzenCore::CallKeyPressFunction(key);
        else if(eventType == BlitzenCore::BlitEventType::KeyReleased)
            BlitzenCore::CallKeyReleaseFunction(key);
        else
        {
            BLIT_ERROR("Unknow event type registered to key function")
            return 0;
        }
        return 1;
    }

    uint8_t OnResize(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, const BlitzenCore::EventContext& data)
    {
        uint32_t newWidth = data.data.ui32[0];
        uint32_t newHeight = data.data.ui32[1];

        Engine::GetEngineInstancePointer()->UpdateWindowSize(newWidth, newHeight);

        return 1;
    }

    uint8_t OnMouseMove(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, const BlitzenCore::EventContext& data)
    {
        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        float deltaTime = static_cast<float>(Engine::GetEngineInstancePointer()->GetDeltaTime());

        camera.transformData.cameraDirty = 1;

        RotateCamera(camera, deltaTime, data.data.si16[1], data.data.si16[0]);

        return 1;
    }

    void RegisterDefaultEvents()
    {
        BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::EngineShutdown, nullptr, OnEvent);

        // Main function called whenever a key press happens
        BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::KeyPressed, nullptr, OnKeyPress);
        // When escape is pressed , Blitzen shuts down
        BlitzenCore::RegisterKeyPressCallback(BlitzenCore::BlitKey::__ESCAPE, CloseOnEscapeKeyPressCallback);
        // When W is pressed the camera moves forward by default
        BlitzenCore::RegisterKeyPressAndReleaseCallback(BlitzenCore::BlitKey::__W, 
        MoveDefaultCameraForwardOnWKeyPressCallback, StopMovingCameraForwardOnWKeyReleaseCallback);
        // When S is pressed the came moves back by default
        BlitzenCore::RegisterKeyPressAndReleaseCallback(BlitzenCore::BlitKey::__S, MoveDefaultCameraBackwardOnSKeyPressCallback, 
        StopMovingCameraBackwardOnSKeyReleaseCallback);
        // When A is pressed the camera moves left by default
        BlitzenCore::RegisterKeyPressAndReleaseCallback(BlitzenCore::BlitKey::__A, MoveDefaultCameraLeftOnAKeyPressCallback, 
        StopMovingCameraLeftOnAKeyReleaseCallback);
        // When D is pressed the came moves right by default
        BlitzenCore::RegisterKeyPressAndReleaseCallback(BlitzenCore::BlitKey::__D, MoveDefaultCameraRightOnDKeyPressCallback, 
        StopMovingCameraRightOnDReleaseCallback);
        // When F1 is frustum culling freezes (debug functionality)
        BlitzenCore::RegisterKeyPressCallback(BlitzenCore::BlitKey::__F1, FreezeFrustumOnF1KeyPressCallback);
        // When F3 is pressed shaders stop performing occlusion culling (debug functionality, does not work on Release)
        BlitzenCore::RegisterKeyPressCallback(BlitzenCore::BlitKey::__F3, ChangeOcclusionCullingState);
        // When F4 is pressed, shader stop performing LOD selection (debug functionality, does not work on Release)
        BlitzenCore::RegisterKeyPressCallback(BlitzenCore::BlitKey::__F4, ChangeLodEnabledState);    

        BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::KeyReleased, nullptr, OnKeyPress);

        BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::WindowResize, nullptr, OnResize);

        BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::MouseMoved, nullptr, OnMouseMove);
    }
}