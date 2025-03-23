#include "blitzenEngine.h"
#include "Core/blitEvents.h"
#include "Renderer/blitRenderer.h"

namespace BlitzenEngine
{
    static uint8_t OnEvent(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, 
    const BlitzenCore::EventContext& data)
    {
        if(eventType == BlitzenCore::BlitEventType::EngineShutdown)
        {
            BLIT_WARN("Engine shutdown event encountered!")
            BlitzenEngine::Engine::GetEngineInstancePointer()->Shutdown();
            return 1; 
        }

        return 0;
    }

    static void CloseOnEscapeKeyPressCallback()
    {
        BlitzenCore::FireEvent<void*>(BlitzenCore::BlitEventType::EngineShutdown, nullptr, {});
    }

    static void MoveDefaultCameraForwardOnWKeyPressCallback()
    {
        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        camera.transformData.cameraDirty = 1;
        camera.transformData.velocity = BlitML::vec3(0.f, 0.f, 1.f);
    }

    static void StopMovingCameraForwardOnWKeyReleaseCallback()
    {
        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        camera.transformData.velocity.z = 0.f;
        if(camera.transformData.velocity.y == 0.f && camera.transformData.velocity.x == 0.f)
        {
            camera.transformData.cameraDirty = 0;
        }
    }

    static void MoveDefaultCameraBackwardOnSKeyPressCallback()
    {
        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        camera.transformData.cameraDirty = 1;
        camera.transformData.velocity = BlitML::vec3(0.f, 0.f, -1.f);
    }

    static void StopMovingCameraBackwardOnSKeyReleaseCallback()
    {
        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        camera.transformData.velocity.z = 0.f;
        if(camera.transformData.velocity.y == 0.f && camera.transformData.velocity.x == 0.f)
        {
            camera.transformData.cameraDirty = 0;
        }
    }

    static void MoveDefaultCameraLeftOnAKeyPressCallback()
    {
        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        camera.transformData.cameraDirty = 1;
        camera.transformData.velocity = BlitML::vec3(-1.f, 0.f, 0.f);
    }

    static void StopMovingCameraLeftOnAKeyReleaseCallback()
    {
        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        camera.transformData.velocity.x = 0.f;
        if (camera.transformData.velocity.y == 0.f && camera.transformData.velocity.z == 0.f)
        {
            camera.transformData.cameraDirty = 0;
        }
    }

    static void MoveDefaultCameraRightOnDKeyPressCallback()
    {
        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        camera.transformData.cameraDirty = 1;
        camera.transformData.velocity = BlitML::vec3(1.f, 0.f, 0.f);
    }

    static void StopMovingCameraRightOnDReleaseCallback()
    {
        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        camera.transformData.velocity.x = 0.f;
        if (camera.transformData.velocity.y == 0.f && camera.transformData.velocity.z == 0.f)
        {
            camera.transformData.cameraDirty = 0;
        }
    }

    static void FreezeFrustumOnF1KeyPressCallback()
    {
        Camera& cam = CameraSystem::GetCameraSystem()->GetCamera();
        cam.transformData.freezeFrustum = !cam.transformData.freezeFrustum;
    }

    static uint8_t OnKeyPress(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, 
        const BlitzenCore::EventContext& data
    )
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

    static void UpdateWindowSize(uint32_t newWidth, uint32_t newHeight)
    {
        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        Engine* pEngine = Engine::GetEngineInstancePointer();

        camera.transformData.windowResize = 1;
        if (newWidth == 0 || newHeight == 0)
        {
            pEngine->Suspend();
            return;
        }
        pEngine->ReActivate();
        UpdateProjection(camera, static_cast<float>(newWidth), static_cast<float>(newHeight));
    }

    static uint8_t OnResize(BlitzenCore::BlitEventType eventType, void* pSender, 
        void* pListener, const BlitzenCore::EventContext& data
    )
    {
        uint32_t newWidth = data.data.ui32[0];
        uint32_t newHeight = data.data.ui32[1];

        UpdateWindowSize(newWidth, newHeight);

        return 1;
    }

    static uint8_t OnMouseMove(BlitzenCore::BlitEventType eventType, void* pSender, 
        void* pListener, const BlitzenCore::EventContext& data
    )
    {
        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        float deltaTime = static_cast<float>(Engine::GetEngineInstancePointer()->GetDeltaTime());

        camera.transformData.cameraDirty = 1;

        RotateCamera(camera, deltaTime, data.data.si16[1], data.data.si16[0]);

        return 1;
    }

	static void OnMouseButtonClickTest(int16_t mouseX, int16_t mouseY)
	{
		BLIT_INFO("Mouse button clicked at %d, %d", mouseX, mouseY);
	}

	static void OnMouseButtonReleaseTest(int16_t mouseX, int16_t mouseY)
	{
		BLIT_INFO("Mouse button released at %d, %d", mouseX, mouseY);
	}

    void RegisterDefaultEvents()
    {
        BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::EngineShutdown, 
            nullptr, OnEvent
        );
        BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::WindowResize, nullptr, OnResize);
        BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::MouseMoved, nullptr, OnMouseMove);

        BlitzenCore::RegisterKeyPressCallback(BlitzenCore::BlitKey::__ESCAPE, 
            CloseOnEscapeKeyPressCallback
        );

        BlitzenCore::RegisterKeyPressAndReleaseCallback(BlitzenCore::BlitKey::__W, 
            MoveDefaultCameraForwardOnWKeyPressCallback, 
            StopMovingCameraForwardOnWKeyReleaseCallback
        );

        BlitzenCore::RegisterKeyPressAndReleaseCallback(BlitzenCore::BlitKey::__S, 
            MoveDefaultCameraBackwardOnSKeyPressCallback, 
            StopMovingCameraBackwardOnSKeyReleaseCallback
        );

        BlitzenCore::RegisterKeyPressAndReleaseCallback(BlitzenCore::BlitKey::__A, 
            MoveDefaultCameraLeftOnAKeyPressCallback, 
            StopMovingCameraLeftOnAKeyReleaseCallback
        );

        BlitzenCore::RegisterKeyPressAndReleaseCallback(BlitzenCore::BlitKey::__D, 
            MoveDefaultCameraRightOnDKeyPressCallback, 
            StopMovingCameraRightOnDReleaseCallback
        );

        BlitzenCore::RegisterKeyReleaseCallback(BlitzenCore::BlitKey::__F1, 
            FreezeFrustumOnF1KeyPressCallback
        );

        BlitzenCore::RegisterMouseButtonPressAndReleaseCallback(BlitzenCore::MouseButton::Left, 
            OnMouseButtonClickTest, OnMouseButtonReleaseTest);
    }
}