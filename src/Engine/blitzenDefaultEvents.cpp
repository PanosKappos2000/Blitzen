#include "blitzenEngine.h"
#include "Core/blitInput.h"
#include "Renderer/blitRenderer.h"
#include "Core/blitTimeManager.h"

namespace BlitzenEngine
{
    static uint8_t OnEvent(void** ppContext, BlitzenCore::BlitEventType eventType)
    {
        if(eventType == BlitzenCore::BlitEventType::EngineShutdown)
        {
            BLIT_WARN("Engine shutdown event encountered!")
            BlitzenWorld_GetEngine(ppContext, BlitzenCore::Ce_WorldContextEngineId)->Shutdown();
            return 1; 
        }

        return 0;
    }

    static void CloseOnEscapeKeyPressCallback(void** ppContext)
    {
        OnEvent(ppContext, BlitzenCore::BlitEventType::EngineShutdown);
    }

    static void MoveDefaultCameraForwardOnWKeyPressCallback(void** ppContext)
    {
        auto& camera{ BlitzenWorld_GetMainCamera(ppContext) };

        camera.transformData.bCameraDirty = true;
        camera.transformData.velocity = BlitML::vec3(0.f, 0.f, 1.f);
    }

    static void StopMovingCameraForwardOnWKeyReleaseCallback(void** ppContext)
    {
        auto& camera{ BlitzenWorld_GetMainCamera(ppContext) };

        camera.transformData.velocity.z = 0.f;
        if(camera.transformData.velocity.y == 0.f && camera.transformData.velocity.x == 0.f)
        {
            camera.transformData.bCameraDirty = false;
        }
    }

    static void MoveDefaultCameraBackwardOnSKeyPressCallback(void** ppContext)
    {
        auto& camera{ BlitzenWorld_GetMainCamera(ppContext) };

        camera.transformData.bCameraDirty = true;
        camera.transformData.velocity = BlitML::vec3(0.f, 0.f, -1.f);
    }

    static void StopMovingCameraBackwardOnSKeyReleaseCallback(void** ppContext)
    {
        auto& camera{ BlitzenWorld_GetMainCamera(ppContext) };

        camera.transformData.velocity.z = 0.f;
        if(camera.transformData.velocity.y == 0.f && camera.transformData.velocity.x == 0.f)
        {
            camera.transformData.bCameraDirty = false;
        }
    }

    static void MoveDefaultCameraLeftOnAKeyPressCallback(void** ppContext)
    {
        auto& camera{ BlitzenWorld_GetMainCamera(ppContext) };

        camera.transformData.bCameraDirty = true;
        camera.transformData.velocity = BlitML::vec3(-1.f, 0.f, 0.f);
    }

    static void StopMovingCameraLeftOnAKeyReleaseCallback(void** ppContext)
    {
        auto& camera{ BlitzenWorld_GetMainCamera(ppContext)};

        camera.transformData.velocity.x = 0.f;
        if (camera.transformData.velocity.y == 0.f && camera.transformData.velocity.z == 0.f)
        {
            camera.transformData.bCameraDirty = false;
        }
    }

    static void MoveDefaultCameraRightOnDKeyPressCallback(void** ppContext)
    {
        auto& camera{ BlitzenWorld_GetMainCamera(ppContext) };

        camera.transformData.bCameraDirty = true;
        camera.transformData.velocity = BlitML::vec3(1.f, 0.f, 0.f);
    }

    static void StopMovingCameraRightOnDReleaseCallback(void** ppContext)
    {
        auto& camera{ BlitzenWorld_GetMainCamera(ppContext) };

        camera.transformData.velocity.x = 0.f;
        if (camera.transformData.velocity.y == 0.f && camera.transformData.velocity.z == 0.f)
        {
            camera.transformData.bCameraDirty = false;
        }
    }

    static void FreezeFrustumOnF1KeyPressCallback(void** ppContext)
    {
        auto& camera{ BlitzenWorld_GetMainCamera(ppContext) };

        camera.transformData.bFreezeFrustum = !camera.transformData.bFreezeFrustum;
    }

    static void UpdateWindowSize(Camera& camera, Engine* pEngine)
    {
        camera.transformData.bWindowResize = 1;

        if (camera.transformData.windowWidth == 0 || camera.transformData.windowHeight == 0)
        {
            pEngine->Suspend();
            return;
        }

        // THIS THING COULD BE WHAT WAS BREAKING LINUX THREADING!!!!
        pEngine->ReActivate();

        UpdateProjection(camera, camera.transformData.windowWidth, camera.transformData.windowHeight);
    }

    static uint8_t OnResize(void** ppContext, BlitzenCore::BlitEventType eventType)
    {
        auto& camera{ BlitzenWorld_GetMainCamera(ppContext) };

        auto pEngine{ BlitzenWorld_GetEngine(ppContext, BlitzenCore::Ce_WorldContextEngineId) };
        
        UpdateWindowSize(camera, pEngine);

        return 1;
    }

    static void OnMouseMove(void** ppContext, int16_t currentX, int16_t currentY, int16_t previousX, int16_t previousY)
    {
        auto& camera{ BlitzenWorld_GetMainCamera(ppContext) };

        auto deltaTime = static_cast<float>(BlitzenCore::BlitzenWorld_GetWorldTimeManager(ppContext)->GetDeltaTime());

        auto xMovement = static_cast<float>(currentX - previousX);
        auto yMovement = static_cast<float>(currentY - previousY);

        RotateCamera(camera, deltaTime, yMovement, xMovement);
    }

	static void OnMouseButtonClickTest(void** ppContext, int16_t mouseX, int16_t mouseY)
	{
		BLIT_INFO("Mouse button clicked at %d, %d", mouseX, mouseY);
	}

	static void OnMouseButtonReleaseTest(void** ppContext, int16_t mouseX, int16_t mouseY)
	{
		BLIT_INFO("Mouse button released at %d, %d", mouseX, mouseY);
	}

    void RegisterDefaultEvents(BlitzenCore::EventSystemState* pEvents, BlitzenCore::InputSystemState* pInputs)
    {
        BlitzenCore::RegisterEvent(pEvents, BlitzenCore::BlitEventType::EngineShutdown, OnEvent);

        BlitzenCore::RegisterEvent(pEvents, BlitzenCore::BlitEventType::WindowResize, OnResize);

        BlitzenCore::RegisterMouseMoveCallback(pInputs, OnMouseMove);

        BlitzenCore::RegisterKeyPressCallback(pInputs, BlitzenCore::BlitKey::__ESCAPE, CloseOnEscapeKeyPressCallback);

        BlitzenCore::RegisterKeyPressAndReleaseCallback(pInputs, BlitzenCore::BlitKey::__W, MoveDefaultCameraForwardOnWKeyPressCallback, StopMovingCameraForwardOnWKeyReleaseCallback);

        BlitzenCore::RegisterKeyPressAndReleaseCallback(pInputs, BlitzenCore::BlitKey::__S, MoveDefaultCameraBackwardOnSKeyPressCallback, StopMovingCameraBackwardOnSKeyReleaseCallback);

        BlitzenCore::RegisterKeyPressAndReleaseCallback(pInputs, BlitzenCore::BlitKey::__A, MoveDefaultCameraLeftOnAKeyPressCallback, StopMovingCameraLeftOnAKeyReleaseCallback);

        BlitzenCore::RegisterKeyPressAndReleaseCallback(pInputs, BlitzenCore::BlitKey::__D, MoveDefaultCameraRightOnDKeyPressCallback, StopMovingCameraRightOnDReleaseCallback);

        BlitzenCore::RegisterKeyReleaseCallback(pInputs, BlitzenCore::BlitKey::__F1, FreezeFrustumOnF1KeyPressCallback);

        BlitzenCore::RegisterMouseButtonPressAndReleaseCallback(pInputs, BlitzenCore::MouseButton::Left, OnMouseButtonClickTest, OnMouseButtonReleaseTest);
    }
}