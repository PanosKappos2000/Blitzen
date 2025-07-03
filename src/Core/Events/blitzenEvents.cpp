#include "blitEvents.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenCore
{
    void ZeroInitializeEventFunctionPointers(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM)
    {
        for (uint32_t i = 0; i < uint8_t(BlitEventType::MaxTypes); ++i)
        {
            SYSTEM->m_eventCallbacks[i] = [](BLIT_STRAIGHTHANDLE, BlitEventType type)->uint8_t {return false; };
        }

        for (auto& controller : SYSTEM->m_controllers)
        {
            controller.InitControllerPFNs();
        }
    }

    void RegisterEvent(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, BlitEventType type, BlitzenWorld::EventCallback eventCallback)
    {
        SYSTEM->m_eventCallbacks[size_t(type)] = eventCallback;
    }

	bool DispatchEventCallback(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, BlitEventType event)
    {
        switch (event)
        {
        case BlitEventType::EngineShutdown:
        {
            auto callback = SYSTEM->m_eventCallbacks[uint32_t(event)];
            return callback(SYSTEM, event);
        }
        case BlitEventType::WindowUpdate:
        {
            auto callback = SYSTEM->m_eventCallbacks[uint32_t(event)];
            if (!callback(SYSTEM, event))
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

    void InputProcessKey(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, BlitKey key, BlitzenCore::FAT_BOOL bPressed)
    {
        auto idx = uint16_t(key);

        // If the state changed, fire callback
        if (SYSTEM->m_currentKeyboard[idx] != bPressed)
        {
            
            BlitEventType event{ BlitEventType::MaxTypes };

            SYSTEM->m_currentKeyboard[idx] = bPressed;
            if (bPressed)
            {
                event = SYSTEM->m_controllers[SYSTEM->m_activeControllerIDX].m_keyPressPFNs[idx](SYSTEM->pWORLD);
            }
            else
            {
                event = SYSTEM->m_controllers[SYSTEM->m_activeControllerIDX].m_keyReleasePFNs[idx](SYSTEM->pWORLD);
            }

            DispatchEventCallback(SYSTEM, event);
            
        }
    }

    void InputProcessButton(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, MouseButton button, BlitzenCore::FAT_BOOL bPressed)
    {
        auto idx = uint8_t(button);

        if (SYSTEM->m_mouseButtonFlags[idx] != bPressed)
        {
            int16_t mouseX{ 0 };
            int16_t mouseY{ 0 };
            BlitzenPlatform::GetSystemMousePos(SYSTEM->pPlatform, mouseX, mouseY);

            SYSTEM->m_mouseButtonFlags[idx] = bPressed;
            if (bPressed)
            {
                SYSTEM->m_controllers[SYSTEM->m_activeControllerIDX].m_mousePressPFNs[idx](SYSTEM->pWORLD, mouseX, mouseY);
            }
            else
            {
                SYSTEM->m_controllers[SYSTEM->m_activeControllerIDX].m_mouseReleasePFNs[idx](SYSTEM->pWORLD, mouseX, mouseY);
            }
        }
    }

    void InputProcessMouseWheel(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, int8_t zDelta)
    {
        SYSTEM->m_controllers[SYSTEM->m_activeControllerIDX].m_mouseWheelPFNs(SYSTEM->pWORLD, zDelta);
    }

    void RegisterKeyPressCallback(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, BlitKey key, KeyPressCallback callback, uint32_t controllerIDX)
    {
        SYSTEM->m_controllers[controllerIDX].m_keyPressPFNs[size_t(key)] = callback;
    }

    void RegisterKeyReleaseCallback(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, BlitKey key, KeyReleaseCallback callback, uint32_t controllerIDX)
    {
        SYSTEM->m_controllers[controllerIDX].m_keyReleasePFNs[size_t(key)] = callback;
    }

    void RegisterKeyPressAndReleaseCallback(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, BlitKey key, KeyPressCallback press, KeyReleaseCallback release, uint32_t controllerIDX)
    {
        SYSTEM->m_controllers[controllerIDX].m_keyPressPFNs[size_t(key)] = press;
        SYSTEM->m_controllers[controllerIDX].m_keyReleasePFNs[size_t(key)] = release;
    }

    void RegisterMouseButtonPressCallback(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, MouseButton button, MouseButtonPressCallback callback, uint32_t controllerIDX)
    {
        SYSTEM->m_controllers[controllerIDX].m_mousePressPFNs[uint8_t(button)] = callback;
    }

    void RegisterMouseButtonReleaseCallback(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, MouseButton button, MouseButtonReleaseCallback callback, uint32_t controllerIDX)
    {
        SYSTEM->m_controllers[controllerIDX].m_mouseReleasePFNs[uint8_t(button)] = callback;
    }

    void RegisterMouseButtonPressAndReleaseCallback(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, MouseButton button, MouseButtonPressCallback press, MouseButtonReleaseCallback release, uint32_t controllerIDX)
    {
        SYSTEM->m_controllers[controllerIDX].m_mousePressPFNs[uint8_t(button)] = press;
        SYSTEM->m_controllers[controllerIDX].m_mouseReleasePFNs[uint8_t(button)] = release;
    }

    void RegisterMouseWheelCallback(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, MouseWheelCallbackType callback, uint32_t controllerIDX)
    {
        SYSTEM->m_controllers[controllerIDX].m_mouseWheelPFNs = callback;
    }

    void RegisterMouseMoveCallback(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, MouseMoveCallbackType callback, uint32_t controllerIDX)
    {
        SYSTEM->m_controllers[controllerIDX].m_mouseMovePFNs = callback;
    }

    void UpdateInput(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, double deltaTime, EditorEventContext* pEditorEvents)
    {
        BlitzenCore::BlitMemCopy(SYSTEM->m_previousKeyboard, SYSTEM->m_currentKeyboard, sizeof(SYSTEM->m_currentKeyboard));

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

    void DispatchRawInput_MOUSE_MOVED(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, int16_t xAxisMovement, int16_t yAxisMovement)
    {
        SYSTEM->m_controllers[SYSTEM->m_activeControllerIDX].m_mouseMovePFNs(SYSTEM->pWORLD, xAxisMovement, yAxisMovement);
    }


    /*************************************                         {DEFAULT CALLBACKS SET BY}                            ************************************************/
    static uint8_t OnShutdown(BLIT_STRAIGHTHANDLE sysHandle, BlitzenCore::BlitEventType eventType)
    {
        if (eventType == BlitzenCore::BlitEventType::EngineShutdown)
        {
			auto SYSTEM{ reinterpret_cast<BlitzenWorld::BLITZEN_SYSTEM_CONTEXT*>(sysHandle) };

            BLIT_WARN("Engine shutdown event encountered!");

            while (SYSTEM->BLITZEN_ENGINE.m_state == BlitzenCore::EngineState::LOADING);// Wait

            SYSTEM->BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::SHUTDOWN;

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

    static uint8_t ResizeEventCallback(BLIT_STRAIGHTHANDLE sysHandle, BlitzenCore::BlitEventType eventType)
    {
		auto SYSTEM{ reinterpret_cast<BlitzenWorld::BLITZEN_SYSTEM_CONTEXT*>(sysHandle) };
        auto& camera{ reinterpret_cast<BlitzenWorld::WORLD_blit*>(SYSTEM->pWORLD)->pCameraContainer->GetMainCamera()};

        if (SYSTEM->BLITZEN_ENGINE.m_state == BlitzenCore::EngineState::LOADING)
        {
            return 0;
        }

        if (SYSTEM->WIDTH == 0 || SYSTEM->HEIGHT == 0)
        {
            SYSTEM->BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::SUSPENDED;
            return 1;
        }

        // Reactivate
        if (SYSTEM->BLITZEN_ENGINE.m_state == BlitzenCore::EngineState::SUSPENDED)
        {
            SYSTEM->BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::RUNNING;
        }

        BlitzenEngine::UpdateProjection(camera, (float)SYSTEM->WIDTH, (float)SYSTEM->HEIGHT);

        BlitML::vec2 hizExtent{ BlitzenEngine::UpdateRendererWindowData(SYSTEM->pWORLD->P_RENDERER.Data(), SYSTEM->WIDTH, SYSTEM->HEIGHT , SYSTEM->pPlatform)};

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

    void RegisterDefaultEvents(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM)
    {
        BlitzenCore::RegisterEvent(SYSTEM, BlitzenCore::BlitEventType::EngineShutdown, OnShutdown);

        BlitzenCore::RegisterEvent(SYSTEM, BlitzenCore::BlitEventType::WindowUpdate, ResizeEventCallback);

        BlitzenCore::RegisterMouseMoveCallback(SYSTEM, OnMouseMove, BlitzenCore::Ce_EngineDefaultGameControllerID);

        BlitzenCore::RegisterKeyPressCallback(SYSTEM, BlitzenCore::BlitKey::__ESCAPE, CloseOnEscapeKeyPressCallback, BlitzenCore::Ce_EditorControllerID);

        BlitzenCore::RegisterKeyPressCallback(SYSTEM, BlitzenCore::BlitKey::__ESCAPE, BringBackEditorOnF10, BlitzenCore::Ce_EngineDefaultGameControllerID);

        BlitzenCore::RegisterKeyPressAndReleaseCallback(SYSTEM, BlitzenCore::BlitKey::__W, MoveDefaultCameraForwardOnWKeyPressCallback, StopMovingCameraForwardOnWKeyReleaseCallback, BlitzenCore::Ce_EngineDefaultGameControllerID);

        BlitzenCore::RegisterKeyPressAndReleaseCallback(SYSTEM, BlitzenCore::BlitKey::__S, MoveDefaultCameraBackwardOnSKeyPressCallback, StopMovingCameraBackwardOnSKeyReleaseCallback, BlitzenCore::Ce_EngineDefaultGameControllerID);

        BlitzenCore::RegisterKeyPressAndReleaseCallback(SYSTEM, BlitzenCore::BlitKey::__A, MoveDefaultCameraLeftOnAKeyPressCallback, StopMovingCameraLeftOnAKeyReleaseCallback, BlitzenCore::Ce_EngineDefaultGameControllerID);

        BlitzenCore::RegisterKeyPressAndReleaseCallback(SYSTEM, BlitzenCore::BlitKey::__D, MoveDefaultCameraRightOnDKeyPressCallback, StopMovingCameraRightOnDReleaseCallback, BlitzenCore::Ce_EngineDefaultGameControllerID);

        BlitzenCore::RegisterMouseButtonPressAndReleaseCallback(SYSTEM, BlitzenCore::MouseButton::Left, OnMouseButtonClickTest, OnMouseButtonReleaseTest, BlitzenCore::Ce_EngineDefaultGameControllerID);

        BlitzenCore::RegisterKeyReleaseCallback(SYSTEM, BlitzenCore::BlitKey::__F8, BringDasherRuntimeDebugWindowOnF9, BlitzenCore::Ce_EngineDefaultGameControllerID);

        BlitzenCore::RegisterKeyReleaseCallback(SYSTEM, BlitzenCore::BlitKey::__F10, BringBackEditorOnF10, BlitzenCore::Ce_EngineDefaultGameControllerID);

#if !defined(BLIT_VK_FORCE)

        BlitzenCore::RegisterKeyReleaseCallback(SYSTEM, BlitzenCore::BlitKey::__F1, FreezeFrustumOnF1KeyPressCallback, BlitzenCore::Ce_EngineDefaultGameControllerID);

        BlitzenCore::RegisterKeyReleaseCallback(SYSTEM, BlitzenCore::BlitKey::__F3, ChangePyramidLevelOnF3ReleaseCallback, BlitzenCore::Ce_EngineDefaultGameControllerID);

        BlitzenCore::RegisterKeyReleaseCallback(SYSTEM, BlitzenCore::BlitKey::__F4, DecreasePyramidLevelOnF4ReleaseCallback, BlitzenCore::Ce_EngineDefaultGameControllerID);

#endif
    }



    /***********************************                        EDITOR EVENTS                    *******************************************************/
    static uint32_t EditorFreezeFrustum(BLIT_STRAIGHTHANDLE context)
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

    static uint32_t EditorEndSceneStart(BLIT_STRAIGHTHANDLE context)
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

    static uint32_t EditorDebugWindowClose(BLIT_STRAIGHTHANDLE context)
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

    void AssignEditorCallbacks(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM)
    {
        SYSTEM->m_editorButtonCallbacks[Ce_ImguiFreezeFrustumButtonID] = EditorFreezeFrustum;

        SYSTEM->m_editorButtonCallbacks[Ce_ImguiSceneStartButtonID] = EditorEndSceneStart;

        SYSTEM->m_editorButtonCallbacks[Ce_ImguiDebugWindowCloseID] = EditorDebugWindowClose;
    }
}