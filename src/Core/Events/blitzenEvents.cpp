#include "blitEvents.h"

namespace BlitzenCore
{
    EventSystem::EventSystem( BlitzenWorld::BlitzenWorldContext& blitzenContext, BlitzenWorld::BlitzenPrivateContext& privateContext) :
        m_blitzenContext{ blitzenContext }, m_privateContext{ privateContext }
    {
        for (uint32_t i = 0; i < uint8_t(BlitEventType::MaxTypes); ++i)
        {
            m_eventCallbacks[i] = [](BlitzenWorld::BlitzenPrivateContext&, BlitEventType type)->uint8_t {return false; };
        }

        for (uint32_t i = 0; i < Ce_KeyCallbackCount; ++i)
        {
            keyPressCallbacks[i] = [](BlitzenWorld::BlitzenWorldContext&)->BlitEventType { return BlitEventType::MaxTypes; };
            keyReleaseCallbacks[i] = [](BlitzenWorld::BlitzenWorldContext&)->BlitEventType { return BlitEventType::MaxTypes; };
        }

        for (uint32_t i = 0; i < uint8_t(MouseButton::MaxButtons); i++)
        {
            mousePressCallbacks[i] = [](BlitzenWorld::BlitzenWorldContext&, int16_t, int16_t)->BlitEventType { return BlitEventType::MaxTypes; };
            mouseReleaseCallbacks[i] = [](BlitzenWorld::BlitzenWorldContext&, int16_t, int16_t)->BlitEventType { return BlitEventType::MaxTypes; };
        }

        mouseMoveCallback = [](BlitzenWorld::BlitzenWorldContext&, int16_t, int16_t, int16_t, int16_t)->BlitEventType { return BlitEventType::MaxTypes; };

        mouseWheelCallback = [](BlitzenWorld::BlitzenWorldContext&, int8_t)->BlitEventType { return BlitEventType::MaxTypes; };
    }

    void RegisterEvent(EventSystem* pContext, BlitEventType type, EventCallback eventCallback)
    {
        pContext->m_eventCallbacks[size_t(type)] = eventCallback;
    }

	bool EventSystem::FireEvent(BlitEventType event)
    {
        switch (event)
        {
        case BlitEventType::EngineShutdown:
        {
            auto callback = m_eventCallbacks[uint32_t(event)];
            return callback(m_privateContext, event);
        }
        case BlitEventType::WindowUpdate:
        {
            auto callback = m_eventCallbacks[uint32_t(event)];
            return callback(m_privateContext, event);
            break;
        }
        case BlitEventType::RendererTransformUpdate:
        {
            //BlitzenEngine::UpdateRendererTransform(m_privateContext.pRenderer, m_blitzenContext.rendererTransformUpdate);
            return true;
        }
        default:
        {
            return false;
        }
        }
    }

    void EventSystem::InputProcessKey(BlitKey key, bool bPressed)
    {
        auto idx = static_cast<uint16_t>(key);

        // If the state changed, fire callback
        if (m_currentKeyboard[idx] != bPressed)
        {
			BlitEventType event{ BlitEventType::MaxTypes };

            m_currentKeyboard[idx] = bPressed;
            if (bPressed)
            {
                event = keyPressCallbacks[idx](m_blitzenContext);
            }
            else
            {
                event = keyReleaseCallbacks[idx](m_blitzenContext);
            }

            FireEvent(event);
        }
    }

    void EventSystem::InputProcessMouseMove(int16_t x, int16_t y)
    {
        if (m_currentMouse.x != x || m_currentMouse.y != y)
        {
            m_previousMouse.x = m_currentMouse.x;
            m_previousMouse.y = m_currentMouse.y;
            m_currentMouse.x = x;
            m_currentMouse.y = y;

            mouseMoveCallback(m_blitzenContext, m_currentMouse.x, m_currentMouse.y, m_previousMouse.x, m_previousMouse.y);
        }
    }

    void EventSystem::InputProcessButton(MouseButton button, bool bPressed)
    {
        auto idx = static_cast<uint8_t>(button);
        // If the state changed, fire callback.
        if (m_currentMouse.buttons[idx] != bPressed)
        {
            m_currentMouse.buttons[idx] = bPressed;
            if (bPressed)
            {
                mousePressCallbacks[idx](m_blitzenContext, m_currentMouse.x, m_currentMouse.y);
            }
            else
            {
                mouseReleaseCallbacks[idx](m_blitzenContext, m_currentMouse.x, m_currentMouse.y);
            }
        }
    }

    void EventSystem::InputProcessMouseWheel(int8_t zDelta)
    {
        mouseWheelCallback(m_blitzenContext, zDelta);
    }

    void RegisterKeyPressCallback(EventSystem* pContext, BlitKey key, KeyPressCallback callback)
    {
        pContext->keyPressCallbacks[size_t(key)] = callback;
    }

    void RegisterKeyReleaseCallback(EventSystem* pContext, BlitKey key, KeyReleaseCallback callback)
    {
        pContext->keyReleaseCallbacks[size_t(key)] = callback;
    }

    void RegisterKeyPressAndReleaseCallback(EventSystem* pContext, BlitKey key, KeyPressCallback press, KeyReleaseCallback release)
    {
        pContext->keyPressCallbacks[size_t(key)] = press;
        pContext->keyReleaseCallbacks[size_t(key)] = release;
    }

    void RegisterMouseButtonPressCallback(EventSystem* pContext, MouseButton button, MouseButtonPressCallback callback)
    {
        pContext->mousePressCallbacks[uint8_t(button)] = callback;
    }

    void RegisterMouseButtonReleaseCallback(EventSystem* pContext, MouseButton button, MouseButtonReleaseCallback callback)
    {
        pContext->mouseReleaseCallbacks[uint8_t(button)] = callback;
    }

    void RegisterMouseButtonPressAndReleaseCallback(EventSystem* pContext, MouseButton button, MouseButtonPressCallback press, MouseButtonReleaseCallback release)
    {
        pContext->mousePressCallbacks[uint8_t(button)] = press;
        pContext->mouseReleaseCallbacks[uint8_t(button)] = release;
    }

    void RegisterMouseWheelCallback(EventSystem* pContext, MouseWheelCallbackType callback)
    {
        pContext->mouseWheelCallback = callback;
    }

    void RegisterMouseMoveCallback(EventSystem* pContext, MouseMoveCallbackType callback)
    {
        pContext->mouseMoveCallback = callback;
    }


    static uint8_t OnEvent(BlitzenWorld::BlitzenPrivateContext& context, BlitzenCore::BlitEventType eventType)
    {
        if (eventType == BlitzenCore::BlitEventType::EngineShutdown)
        {
            BLIT_WARN("Engine shutdown event encountered!");
            auto pBlitState{ context.pEngineState };
            *pBlitState = BlitzenCore::EngineState::SHUTDOWN;

            return 1;
        }

        return 0;
    }

    static BlitEventType CloseOnEscapeKeyPressCallback(BlitzenWorld::BlitzenWorldContext& context)
    {
		return BlitEventType::EngineShutdown;
    }

    static BlitEventType MoveDefaultCameraForwardOnWKeyPressCallback(BlitzenWorld::BlitzenWorldContext& blitzenContext)
    {
        auto& camera{ blitzenContext.pCameraContainer->GetMainCamera() };

        camera.transformData.bCameraDirty = true;
        camera.transformData.velocity = BlitML::vec3(0.f, 0.f, 1.f);

        return BlitEventType::MaxTypes;
    }

    static BlitEventType StopMovingCameraForwardOnWKeyReleaseCallback(BlitzenWorld::BlitzenWorldContext& blitzenContext)
    {
        auto& camera{ blitzenContext.pCameraContainer->GetMainCamera() };

        camera.transformData.velocity.z = 0.f;
        if (camera.transformData.velocity.y == 0.f && camera.transformData.velocity.x == 0.f)
        {
            camera.transformData.bCameraDirty = false;
        }

        return BlitEventType::MaxTypes;
    }

    static BlitEventType MoveDefaultCameraBackwardOnSKeyPressCallback(BlitzenWorld::BlitzenWorldContext& blitzenContext)
    {
        auto& camera{ blitzenContext.pCameraContainer->GetMainCamera() };

        camera.transformData.bCameraDirty = true;
        camera.transformData.velocity = BlitML::vec3(0.f, 0.f, -1.f);

        return BlitEventType::MaxTypes;
    }

    static BlitEventType StopMovingCameraBackwardOnSKeyReleaseCallback(BlitzenWorld::BlitzenWorldContext& blitzenContext)
    {
        auto& camera{ blitzenContext.pCameraContainer->GetMainCamera() };

        camera.transformData.velocity.z = 0.f;
        if (camera.transformData.velocity.y == 0.f && camera.transformData.velocity.x == 0.f)
        {
            camera.transformData.bCameraDirty = false;
        }

        return BlitEventType::MaxTypes;
    }

    static BlitEventType MoveDefaultCameraLeftOnAKeyPressCallback(BlitzenWorld::BlitzenWorldContext& blitzenContext)
    {
        auto& camera{ blitzenContext.pCameraContainer->GetMainCamera() };

        camera.transformData.bCameraDirty = true;
        camera.transformData.velocity = BlitML::vec3(-1.f, 0.f, 0.f);

        return BlitEventType::MaxTypes;
    }

    static BlitEventType StopMovingCameraLeftOnAKeyReleaseCallback(BlitzenWorld::BlitzenWorldContext& blitzenContext)
    {
        auto& camera{ blitzenContext.pCameraContainer->GetMainCamera() };

        camera.transformData.velocity.x = 0.f;
        if (camera.transformData.velocity.y == 0.f && camera.transformData.velocity.z == 0.f)
        {
            camera.transformData.bCameraDirty = false;
        }

        return BlitEventType::MaxTypes;
    }

    static BlitEventType MoveDefaultCameraRightOnDKeyPressCallback(BlitzenWorld::BlitzenWorldContext& blitzenContext)
    {
        auto& camera{ blitzenContext.pCameraContainer->GetMainCamera() };

        camera.transformData.bCameraDirty = true;
        camera.transformData.velocity = BlitML::vec3(1.f, 0.f, 0.f);

        return BlitEventType::MaxTypes;
    }

    static BlitEventType StopMovingCameraRightOnDReleaseCallback(BlitzenWorld::BlitzenWorldContext& blitzenContext)
    {
        auto& camera{ blitzenContext.pCameraContainer->GetMainCamera() };

        camera.transformData.velocity.x = 0.f;
        if (camera.transformData.velocity.y == 0.f && camera.transformData.velocity.z == 0.f)
        {
            camera.transformData.bCameraDirty = false;
        }

        return BlitEventType::MaxTypes;
    }

    static BlitEventType FreezeFrustumOnF1KeyPressCallback(BlitzenWorld::BlitzenWorldContext& blitzenContext)
    {
        auto& camera{ blitzenContext.pCameraContainer->GetMainCamera() };

        camera.transformData.bFreezeFrustum = !camera.transformData.bFreezeFrustum;

        return BlitEventType::MaxTypes;
    }

    static BlitEventType ChangePyramidLevelOnF3ReleaseCallback(BlitzenWorld::BlitzenWorldContext& blitzenContext)
    {
        auto& camera{ blitzenContext.pCameraContainer->GetMainCamera() };

        if (camera.transformData.debugPyramidLevel >= 16)
        {
            camera.transformData.debugPyramidLevel = 0;
        }
        else
        {
            camera.transformData.debugPyramidLevel++;
        }

        return BlitEventType::MaxTypes;
    }

    static BlitEventType DecreasePyramidLevelOnF4ReleaseCallback(BlitzenWorld::BlitzenWorldContext& blitzenContext)
    {
        auto& camera{ blitzenContext.pCameraContainer->GetMainCamera() };

        if (camera.transformData.debugPyramidLevel != 0)
        {
            camera.transformData.debugPyramidLevel--;
        }

        return BlitEventType::MaxTypes;
    }

    static uint8_t ResizeEventCallback(BlitzenWorld::BlitzenPrivateContext& context, BlitzenCore::BlitEventType eventType)
    {
        auto& camera{ reinterpret_cast<BlitzenWorld::BlitzenWorldContext*>(context.pBlitzenContext)->pCameraContainer->GetMainCamera()};
        auto pBlitState{ context.pEngineState };

        if (*pBlitState == BlitzenCore::EngineState::LOADING)
        {
            return 0;
        }

        camera.transformData.bWindowResize = 1;

        if (camera.transformData.windowWidth == 0 || camera.transformData.windowHeight == 0)
        {
            *pBlitState = BlitzenCore::EngineState::SUSPENDED;
            return 1;
        }

        // Reactivate
        if (*pBlitState == BlitzenCore::EngineState::SUSPENDED)
        {
            *pBlitState = BlitzenCore::EngineState::RUNNING;
        }

        BlitzenEngine::UpdateProjection(camera, camera.transformData.windowWidth, camera.transformData.windowHeight);

        return 1;
    }

    static BlitEventType OnMouseMove(BlitzenWorld::BlitzenWorldContext& blitzenContext, int16_t currentX, int16_t currentY, int16_t previousX, int16_t previousY)
    {
        auto& camera{ blitzenContext.pCameraContainer->GetMainCamera() };

        auto deltaTime = float(blitzenContext.pCoreClock->m_deltaTime);

        auto xMovement = float(currentX - previousX);
        auto yMovement = float(currentY - previousY);

        BlitzenEngine::RotateCamera(camera, deltaTime, yMovement, xMovement);

        return BlitEventType::MaxTypes;
    }

    static BlitEventType OnMouseButtonClickTest(BlitzenWorld::BlitzenWorldContext& blitzenContext, int16_t mouseX, int16_t mouseY)
    {
        BLIT_INFO("Mouse button clicked at %d, %d", mouseX, mouseY);

        return BlitEventType::MaxTypes;
    }

    static BlitEventType OnMouseButtonReleaseTest(BlitzenWorld::BlitzenWorldContext& blitzenContext, int16_t mouseX, int16_t mouseY)
    {
        BLIT_INFO("Mouse button released at %d, %d", mouseX, mouseY);

        return BlitEventType::MaxTypes;
    }

    void RegisterDefaultEvents(EventSystem* pEvents)
    {
        BlitzenCore::RegisterEvent(pEvents, BlitzenCore::BlitEventType::EngineShutdown, OnEvent);

        BlitzenCore::RegisterEvent(pEvents, BlitzenCore::BlitEventType::WindowUpdate, ResizeEventCallback);

        BlitzenCore::RegisterMouseMoveCallback(pEvents, OnMouseMove);

        BlitzenCore::RegisterKeyPressCallback(pEvents, BlitzenCore::BlitKey::__ESCAPE, CloseOnEscapeKeyPressCallback);

        BlitzenCore::RegisterKeyPressAndReleaseCallback(pEvents, BlitzenCore::BlitKey::__W, MoveDefaultCameraForwardOnWKeyPressCallback, StopMovingCameraForwardOnWKeyReleaseCallback);

        BlitzenCore::RegisterKeyPressAndReleaseCallback(pEvents, BlitzenCore::BlitKey::__S, MoveDefaultCameraBackwardOnSKeyPressCallback, StopMovingCameraBackwardOnSKeyReleaseCallback);

        BlitzenCore::RegisterKeyPressAndReleaseCallback(pEvents, BlitzenCore::BlitKey::__A, MoveDefaultCameraLeftOnAKeyPressCallback, StopMovingCameraLeftOnAKeyReleaseCallback);

        BlitzenCore::RegisterKeyPressAndReleaseCallback(pEvents, BlitzenCore::BlitKey::__D, MoveDefaultCameraRightOnDKeyPressCallback, StopMovingCameraRightOnDReleaseCallback);

        BlitzenCore::RegisterKeyReleaseCallback(pEvents, BlitzenCore::BlitKey::__F1, FreezeFrustumOnF1KeyPressCallback);

        BlitzenCore::RegisterKeyReleaseCallback(pEvents, BlitzenCore::BlitKey::__F3, ChangePyramidLevelOnF3ReleaseCallback);

        BlitzenCore::RegisterKeyReleaseCallback(pEvents, BlitzenCore::BlitKey::__F4, DecreasePyramidLevelOnF4ReleaseCallback);

        BlitzenCore::RegisterMouseButtonPressAndReleaseCallback(pEvents, BlitzenCore::MouseButton::Left, OnMouseButtonClickTest, OnMouseButtonReleaseTest);
    }
}