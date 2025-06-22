#include "blitEvents.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenCore
{
    EventSystem::EventSystem(BlitzenWorld::WORLD_blit* pWORLD, BlitzenWorld::BLITZEN_SYSTEM_CONTEXT& systemContext) :
        m_pWorldContext{ pWORLD }, m_systemContext{ systemContext }
    {
        for (uint32_t i = 0; i < uint8_t(BlitEventType::MaxTypes); ++i)
        {
            m_eventCallbacks[i] = [](BlitzenWorld::BLITZEN_SYSTEM_CONTEXT&, BlitEventType type)->uint8_t {return false; };
        }

        for (auto& controller : m_controllers)
        {
            InitControllerPFNs(controller);
        }
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
            return callback(m_systemContext, event);
        }
        case BlitEventType::WindowUpdate:
        {
            auto callback = m_eventCallbacks[uint32_t(event)];
            if (!callback(m_systemContext, event))
            {
                BLIT_ERROR("Failure to update window");
                return false;
            }

            return true;
        }
        case BlitEventType::BringBackEditor:
        {
#if defined(DASHER_JOIN)
            if (m_systemContext.pDasher->m_state == DASHER_STATE::BLIT_RENDERER_FULL_DASHER_EDITOR_NO_JOIN && m_systemContext.BLITZEN_ENGINE.m_state == EngineState::RUNNING_EDITOR_NO_START)
            {
                m_systemContext.pDasher->m_state = DASHER_STATE::DASHER_EDITOR_FULL_BLIT_RENDERER_IDLE;
                m_systemContext.BLITZEN_ENGINE.m_state = EngineState::RUNNING;
                m_activeControllerIDX = Ce_EditorControllerID;

                return true;
            }
#endif

            return false;
        }
        case BlitEventType::BringDasherRuntimeDebugWindow:
        {
#if defined(DASHER_JOIN)
            if (m_systemContext.pDasher->m_state == DASHER_STATE::BLIT_RENDERER_FULL_DASHER_EDITOR_NO_JOIN && m_systemContext.BLITZEN_ENGINE.m_state == EngineState::RUNNING_EDITOR_NO_START)
            {
                m_systemContext.pDasher->m_state = DASHER_STATE::DASHER_EDITOR_DEBUG_RENDERER;
                m_systemContext.BLITZEN_ENGINE.m_state = EngineState::RUNNING;
                m_activeControllerIDX = Ce_EditorControllerID;

                return true;
            }
#endif

            return false;
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

    void EventSystem::InputProcessKey(BlitKey key, BlitzenCore::FAT_BOOL bPressed)
    {
        auto idx = uint16_t(key);

        // If the state changed, fire callback
        if (m_currentKeyboard[idx] != bPressed)
        {
			BlitEventType event{ BlitEventType::MaxTypes };

            m_currentKeyboard[idx] = bPressed;
            if (bPressed)
            {
                event = m_controllers[m_activeControllerIDX].m_keyPressPFNs[idx](m_pWorldContext);
            }
            else
            {
                event = m_controllers[m_activeControllerIDX].m_keyReleasePFNs[idx](m_pWorldContext);
            }

            FireEvent(event);
        }
    }

    void EventSystem::InputProcessButton(MouseButton button, BlitzenCore::FAT_BOOL bPressed)
    {
        auto idx = uint8_t(button);

        if (m_mouseButtonFlags[idx] != bPressed)
        {
            int16_t mouseX{ 0 };
            int16_t mouseY{ 0 };
            BlitzenPlatform::GetSystemMousePos(m_systemContext.pPlatform, mouseX, mouseY);

            m_mouseButtonFlags[idx] = bPressed;
            if (bPressed)
            {
                m_controllers[m_activeControllerIDX].m_mousePressPFNs[idx](m_pWorldContext, mouseX, mouseY);
            }
            else
            {
                m_controllers[m_activeControllerIDX].m_mouseReleasePFNs[idx](m_pWorldContext, mouseX, mouseY);
            }
        }
    }

    void EventSystem::InputProcessMouseWheel(int8_t zDelta)
    {
        m_controllers[m_activeControllerIDX].m_mouseWheelPFNs(m_pWorldContext, zDelta);
    }

    void RegisterKeyPressCallback(EventSystem* pContext, BlitKey key, KeyPressCallback callback, uint32_t controllerIDX)
    {
        pContext->m_controllers[controllerIDX].m_keyPressPFNs[size_t(key)] = callback;
    }

    void RegisterKeyReleaseCallback(EventSystem* pContext, BlitKey key, KeyReleaseCallback callback, uint32_t controllerIDX)
    {
        pContext->m_controllers[controllerIDX].m_keyReleasePFNs[size_t(key)] = callback;
    }

    void RegisterKeyPressAndReleaseCallback(EventSystem* pContext, BlitKey key, KeyPressCallback press, KeyReleaseCallback release, uint32_t controllerIDX)
    {
        pContext->m_controllers[controllerIDX].m_keyPressPFNs[size_t(key)] = press;
        pContext->m_controllers[controllerIDX].m_keyReleasePFNs[size_t(key)] = release;
    }

    void RegisterMouseButtonPressCallback(EventSystem* pContext, MouseButton button, MouseButtonPressCallback callback, uint32_t controllerIDX)
    {
        pContext->m_controllers[controllerIDX].m_mousePressPFNs[uint8_t(button)] = callback;
    }

    void RegisterMouseButtonReleaseCallback(EventSystem* pContext, MouseButton button, MouseButtonReleaseCallback callback, uint32_t controllerIDX)
    {
        pContext->m_controllers[controllerIDX].m_mouseReleasePFNs[uint8_t(button)] = callback;
    }

    void RegisterMouseButtonPressAndReleaseCallback(EventSystem* pContext, MouseButton button, MouseButtonPressCallback press, MouseButtonReleaseCallback release, uint32_t controllerIDX)
    {
        pContext->m_controllers[controllerIDX].m_mousePressPFNs[uint8_t(button)] = press;
        pContext->m_controllers[controllerIDX].m_mouseReleasePFNs[uint8_t(button)] = release;
    }

    void RegisterMouseWheelCallback(EventSystem* pContext, MouseWheelCallbackType callback, uint32_t controllerIDX)
    {
        pContext->m_controllers[controllerIDX].m_mouseWheelPFNs = callback;
    }

    void RegisterMouseMoveCallback(EventSystem* pContext, MouseMoveCallbackType callback, uint32_t controllerIDX)
    {
        pContext->m_controllers[controllerIDX].m_mouseMovePFNs = callback;
    }

    void EventSystem::UpdateInput(double deltaTime, EditorEventContext* pEditorEvents)
    {
        BlitzenCore::BlitMemCopy(m_previousKeyboard, m_currentKeyboard, sizeof(m_currentKeyboard));

#if defined(DASHER_JOIN) && defined(DASHER_USE_DEAR)

        if (pEditorEvents != nullptr)
        {
            auto& editorEvent{ pEditorEvents->m_events[pEditorEvents->m_currentID] };

            switch (editorEvent.m_type)
            {
            case EditorEventType::BUTTON_CLICK:
            {
                m_activeControllerIDX = m_editorButtonCallbacks[editorEvent.m_eventTypeID](m_systemContext);

                BLIT_ASSERT_MESSAGE(m_activeControllerIDX < Ce_MaxControllerCount, "Editor callback returned a controller index, which is bigger than the max count");

                editorEvent.m_type = EditorEventType::NO_EVENT;

                pEditorEvents->m_currentID = (pEditorEvents->m_currentID + 1) % Ce_EditorButtonEventTypeCount;

                break;
            }
            default:
            {
                break;
            }
            }
        }
#endif
    }

    void EventSystem::DispatchRawInput_MOUSE_MOVED(int16_t xAxisMovement, int16_t yAxisMovement)
    {
        m_controllers[m_activeControllerIDX].m_mouseMovePFNs(m_pWorldContext, xAxisMovement, yAxisMovement);
    }


    /*************************************                         {DEFAULT CALLBACKS SET BY}                            ************************************************/
    static uint8_t OnShutdown(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT& context, BlitzenCore::BlitEventType eventType)
    {
        if (eventType == BlitzenCore::BlitEventType::EngineShutdown)
        {
            BLIT_WARN("Engine shutdown event encountered!");

            while (context.BLITZEN_ENGINE.m_state == BlitzenCore::EngineState::LOADING);// Wait

            context.BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::SHUTDOWN;

            return 1;
        }

        return 0;
    }

    static BlitEventType CloseOnEscapeKeyPressCallback(BlitzenWorld::WORLD_blit* context)
    {
		return BlitEventType::EngineShutdown;
    }

    static BlitEventType MoveDefaultCameraForwardOnWKeyPressCallback(BlitzenWorld::WORLD_blit* blitzenContext)
    {
        auto& camera{ blitzenContext->pCameraContainer->GetMainCamera() };

        camera.transformData.bCameraDirty = true;
        camera.transformData.velocity = BlitML::vec3(0.f, 0.f, 1.f);

        return BlitEventType::MaxTypes;
    }

    static BlitEventType StopMovingCameraForwardOnWKeyReleaseCallback(BlitzenWorld::WORLD_blit* blitzenContext)
    {
        auto& camera{ blitzenContext->pCameraContainer->GetMainCamera() };

        camera.transformData.velocity.z = 0.f;
        if (camera.transformData.velocity.y == 0.f && camera.transformData.velocity.x == 0.f)
        {
            camera.transformData.bCameraDirty = false;
        }

        return BlitEventType::MaxTypes;
    }

    static BlitEventType MoveDefaultCameraBackwardOnSKeyPressCallback(BlitzenWorld::WORLD_blit* blitzenContext)
    {
        auto& camera{ blitzenContext->pCameraContainer->GetMainCamera() };

        camera.transformData.bCameraDirty = true;
        camera.transformData.velocity = BlitML::vec3(0.f, 0.f, -1.f);

        return BlitEventType::MaxTypes;
    }

    static BlitEventType StopMovingCameraBackwardOnSKeyReleaseCallback(BlitzenWorld::WORLD_blit* blitzenContext)
    {
        auto& camera{ blitzenContext->pCameraContainer->GetMainCamera() };

        camera.transformData.velocity.z = 0.f;
        if (camera.transformData.velocity.y == 0.f && camera.transformData.velocity.x == 0.f)
        {
            camera.transformData.bCameraDirty = false;
        }

        return BlitEventType::MaxTypes;
    }

    static BlitEventType MoveDefaultCameraLeftOnAKeyPressCallback(BlitzenWorld::WORLD_blit* blitzenContext)
    {
        auto& camera{ blitzenContext->pCameraContainer->GetMainCamera() };

        camera.transformData.bCameraDirty = true;
        camera.transformData.velocity = BlitML::vec3(-1.f, 0.f, 0.f);

        return BlitEventType::MaxTypes;
    }

    static BlitEventType StopMovingCameraLeftOnAKeyReleaseCallback(BlitzenWorld::WORLD_blit* blitzenContext)
    {
        auto& camera{ blitzenContext->pCameraContainer->GetMainCamera() };

        camera.transformData.velocity.x = 0.f;
        if (camera.transformData.velocity.y == 0.f && camera.transformData.velocity.z == 0.f)
        {
            camera.transformData.bCameraDirty = false;
        }

        return BlitEventType::MaxTypes;
    }

    static BlitEventType MoveDefaultCameraRightOnDKeyPressCallback(BlitzenWorld::WORLD_blit* blitzenContext)
    {
        auto& camera{ blitzenContext->pCameraContainer->GetMainCamera() };

        camera.transformData.bCameraDirty = true;
        camera.transformData.velocity = BlitML::vec3(1.f, 0.f, 0.f);

        return BlitEventType::MaxTypes;
    }

    static BlitEventType StopMovingCameraRightOnDReleaseCallback(BlitzenWorld::WORLD_blit* blitzenContext)
    {
        auto& camera{ blitzenContext->pCameraContainer->GetMainCamera() };

        camera.transformData.velocity.x = 0.f;
        if (camera.transformData.velocity.y == 0.f && camera.transformData.velocity.z == 0.f)
        {
            camera.transformData.bCameraDirty = false;
        }

        return BlitEventType::MaxTypes;
    }

    static BlitEventType FreezeFrustumOnF1KeyPressCallback(BlitzenWorld::WORLD_blit* blitzenContext)
    {
        auto& camera{ blitzenContext->pCameraContainer->GetMainCamera() };

        camera.transformData.bFreezeFrustum = !camera.transformData.bFreezeFrustum;

        return BlitEventType::MaxTypes;
    }

    static BlitEventType ChangePyramidLevelOnF3ReleaseCallback(BlitzenWorld::WORLD_blit* blitzenContext)
    {
        auto& camera{ blitzenContext->pCameraContainer->GetMainCamera() };

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

    static BlitEventType DecreasePyramidLevelOnF4ReleaseCallback(BlitzenWorld::WORLD_blit* blitzenContext)
    {
        auto& camera{ blitzenContext->pCameraContainer->GetMainCamera() };

        if (camera.transformData.debugPyramidLevel != 0)
        {
            camera.transformData.debugPyramidLevel--;
        }

        return BlitEventType::MaxTypes;
    }

    static BlitEventType BringBackEditorOnF10(BlitzenWorld::WORLD_blit* blitzenContext)
    {
        return BlitEventType::BringBackEditor;
    }

    static BlitEventType BringDasherRuntimeDebugWindowOnF9(BlitzenWorld::WORLD_blit* blitzenContext)
    {
        return BlitEventType::BringDasherRuntimeDebugWindow;
    }

    static uint8_t ResizeEventCallback(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT& context, BlitzenCore::BlitEventType eventType)
    {
        auto& camera{ reinterpret_cast<BlitzenWorld::WORLD_blit*>(context.pWORLD)->pCameraContainer->GetMainCamera()};

        if (context.BLITZEN_ENGINE.m_state == BlitzenCore::EngineState::LOADING)
        {
            return 0;
        }

        if (context.WIDTH == 0 || context.HEIGHT == 0)
        {
            context.BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::SUSPENDED;
            return 1;
        }

        // Reactivate
        if (context.BLITZEN_ENGINE.m_state == BlitzenCore::EngineState::SUSPENDED)
        {
            context.BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::RUNNING;
        }

        BlitzenEngine::UpdateProjection(camera, (float)context.WIDTH, (float)context.HEIGHT);

        BlitML::vec2 hizExtent{ context.pWORLD->P_RENDERER->UpdateWindow(context.WIDTH, context.HEIGHT , context.pPlatform) };

#if defined(DASHER_JOIN)
        context.pDasher->UpdateWindowSize(context.WIDTH, context.HEIGHT);
#endif

        camera.viewData.pyramidWidth = hizExtent.x;
        camera.viewData.pyramidHeight = hizExtent.y;

        return 1;
    }

    static BlitEventType OnMouseMove(BlitzenWorld::WORLD_blit* blitzenContext, int16_t xAxisMovement, int16_t yAxisMovement)
    {
        auto& camera{ blitzenContext->pCameraContainer->GetMainCamera() };

        auto deltaTime = float(blitzenContext->deltaTime);

        BlitzenEngine::RotateCamera(camera, deltaTime, yAxisMovement, xAxisMovement);

        return BlitEventType::MaxTypes;
    }

    static BlitEventType OnMouseButtonClickTest(BlitzenWorld::WORLD_blit* blitzenContext, int16_t mouseX, int16_t mouseY)
    {
        BLIT_INFO("Mouse button clicked at %d, %d", mouseX, mouseY);

        return BlitEventType::MaxTypes;
    }

    static BlitEventType OnMouseButtonReleaseTest(BlitzenWorld::WORLD_blit* blitzenContext, int16_t mouseX, int16_t mouseY)
    {
        BLIT_INFO("Mouse button released at %d, %d", mouseX, mouseY);

        return BlitEventType::MaxTypes;
    }

    void RegisterDefaultEvents(EventSystem* pEvents)
    {
        BlitzenCore::RegisterEvent(pEvents, BlitzenCore::BlitEventType::EngineShutdown, OnShutdown);

        BlitzenCore::RegisterEvent(pEvents, BlitzenCore::BlitEventType::WindowUpdate, ResizeEventCallback);

        BlitzenCore::RegisterMouseMoveCallback(pEvents, OnMouseMove, BlitzenCore::Ce_EngineDefaultGameControllerID);

        BlitzenCore::RegisterKeyPressCallback(pEvents, BlitzenCore::BlitKey::__ESCAPE, CloseOnEscapeKeyPressCallback, BlitzenCore::Ce_EditorControllerID);

        BlitzenCore::RegisterKeyPressCallback(pEvents, BlitzenCore::BlitKey::__ESCAPE, BringBackEditorOnF10, BlitzenCore::Ce_EngineDefaultGameControllerID);

        BlitzenCore::RegisterKeyPressAndReleaseCallback(pEvents, BlitzenCore::BlitKey::__W, MoveDefaultCameraForwardOnWKeyPressCallback, StopMovingCameraForwardOnWKeyReleaseCallback, BlitzenCore::Ce_EngineDefaultGameControllerID);

        BlitzenCore::RegisterKeyPressAndReleaseCallback(pEvents, BlitzenCore::BlitKey::__S, MoveDefaultCameraBackwardOnSKeyPressCallback, StopMovingCameraBackwardOnSKeyReleaseCallback, BlitzenCore::Ce_EngineDefaultGameControllerID);

        BlitzenCore::RegisterKeyPressAndReleaseCallback(pEvents, BlitzenCore::BlitKey::__A, MoveDefaultCameraLeftOnAKeyPressCallback, StopMovingCameraLeftOnAKeyReleaseCallback, BlitzenCore::Ce_EngineDefaultGameControllerID);

        BlitzenCore::RegisterKeyPressAndReleaseCallback(pEvents, BlitzenCore::BlitKey::__D, MoveDefaultCameraRightOnDKeyPressCallback, StopMovingCameraRightOnDReleaseCallback, BlitzenCore::Ce_EngineDefaultGameControllerID);

        BlitzenCore::RegisterMouseButtonPressAndReleaseCallback(pEvents, BlitzenCore::MouseButton::Left, OnMouseButtonClickTest, OnMouseButtonReleaseTest, BlitzenCore::Ce_EngineDefaultGameControllerID);

        BlitzenCore::RegisterKeyReleaseCallback(pEvents, BlitzenCore::BlitKey::__F8, BringDasherRuntimeDebugWindowOnF9, BlitzenCore::Ce_EngineDefaultGameControllerID);

        BlitzenCore::RegisterKeyReleaseCallback(pEvents, BlitzenCore::BlitKey::__F10, BringBackEditorOnF10, BlitzenCore::Ce_EngineDefaultGameControllerID);

#if !defined(BLIT_VK_FORCE)

        BlitzenCore::RegisterKeyReleaseCallback(pEvents, BlitzenCore::BlitKey::__F1, FreezeFrustumOnF1KeyPressCallback, BlitzenCore::Ce_EngineDefaultGameControllerID);

        BlitzenCore::RegisterKeyReleaseCallback(pEvents, BlitzenCore::BlitKey::__F3, ChangePyramidLevelOnF3ReleaseCallback, BlitzenCore::Ce_EngineDefaultGameControllerID);

        BlitzenCore::RegisterKeyReleaseCallback(pEvents, BlitzenCore::BlitKey::__F4, DecreasePyramidLevelOnF4ReleaseCallback, BlitzenCore::Ce_EngineDefaultGameControllerID);

#endif
    }



    /***********************************                        EDITOR EVENTS                    *******************************************************/
    static uint32_t EditorFreezeFrustum(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT& context)
    {
#if defined(DASHER_JOIN)

        if (context.BLITZEN_ENGINE.m_state != EngineState::RUNNING)
        {
            return BlitzenCore::Ce_EditorControllerID;
        }

        auto pBlitzenContext{ reinterpret_cast<BlitzenWorld::WORLD_blit*>(context.pWORLD) };
        auto& camera{ pBlitzenContext->pCameraContainer->GetMainCamera() };

        camera.transformData.bFreezeFrustum = !camera.transformData.bFreezeFrustum;

        context.BLITZEN_ENGINE.m_state = EngineState::RUNNING_EDITOR_NO_START;

        context.pDasher->m_state = DASHER_STATE::BLIT_RENDERER_FULL_DASHER_EDITOR_NO_JOIN;
        
        return BlitzenCore::Ce_EngineDefaultGameControllerID;

#else
        BLIT_ERROR("This should not be called when the editor is inactive");
        return 0;
#endif
    }

    static uint32_t EditorEndSceneStart(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT& context)
    {
#if defined(DASHER_JOIN)

        if (context.BLITZEN_ENGINE.m_state != EngineState::RUNNING)
        {
            return BlitzenCore::Ce_EditorControllerID;
        }

        context.BLITZEN_ENGINE.m_state = EngineState::RUNNING_EDITOR_NO_START;

        context.pDasher->m_state = DASHER_STATE::BLIT_RENDERER_FULL_DASHER_EDITOR_NO_JOIN;

        return BlitzenCore::Ce_EngineDefaultGameControllerID;

#else
        BLIT_ERROR("This should not be called when the editor is inactive");
        return 0;
#endif
    }

    static uint32_t EditorDebugWindowClose(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT& context)
    {
#if defined(DASHER_JOIN)

        if (context.BLITZEN_ENGINE.m_state != EngineState::RUNNING)
        {
            return BlitzenCore::Ce_EditorControllerID;
        }
        
        context.BLITZEN_ENGINE.m_state = EngineState::RUNNING_EDITOR_NO_START;

        context.pDasher->m_state = DASHER_STATE::BLIT_RENDERER_FULL_DASHER_EDITOR_NO_JOIN;

        return BlitzenCore::Ce_EngineDefaultGameControllerID;

#else
        BLIT_ERROR("This should not be called when the editor is inactive");
        return 0;
#endif
    }

    void AssignEditorCallbacks(EventSystem* pContext)
    {
        pContext->m_editorButtonCallbacks[Ce_ImguiFreezeFrustumButtonID] = EditorFreezeFrustum;

        pContext->m_editorButtonCallbacks[Ce_ImguiSceneStartButtonID] = EditorEndSceneStart;

        pContext->m_editorButtonCallbacks[Ce_ImguiDebugWindowCloseID] = EditorDebugWindowClose;
    }
}