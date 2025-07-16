#include "blitEvents.h"
#include "Core/DbLog/blitAssert.h"
#include "Core/BlitzenWorld/blitzenUserInterface.h"

namespace BlitzenCore
{
    void DispatchUserEvents(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM)
    {
        if (SYSTEM->BLITZEN_ENGINE.m_state == EngineState::RUNNING)
        {
            if (SYSTEM->mMouseDeltaXAxis != 0 || SYSTEM->mMouseDeltaYAxis != 0)
            {
                DispatchRawInput_MOUSE_MOVED(SYSTEM, SYSTEM->mMouseDeltaXAxis, SYSTEM->mMouseDeltaYAxis);
                SYSTEM->mMouseDeltaXAxis = 0;
                SYSTEM->mMouseDeltaYAxis = 0;
            }
        }
    }

    void ZeroInitializeEventFunctionPointers(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM)
    {
        for (uint32_t i = 0; i < uint8_t(BlitEventType::MaxTypes); ++i)
        {
            SYSTEM->m_eventCallbacks[i] = [](BLIT_STRAIGHTHANDLE, BlitEventType type)->uint8_t {return false; };
        }

        for (uint32_t ctrlID = 0; ctrlID < CE_STARTING_CONTROLLER_COUNT; ++ctrlID)
        {
            SYSTEM->m_controllers[ctrlID].InitControllerPFNs();
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
        case BlitEventType::FreezeFrustum:
        {
            SYSTEM->pWORLD->m_cameras[SYSTEM->pWORLD->m_activeCameraIDX].transformData.bFreezeFrustum = !SYSTEM->pWORLD->m_cameras[SYSTEM->pWORLD->m_activeCameraIDX].transformData.bFreezeFrustum;
            return true;
        }
        default:
        {
            return false;
        }
        }
    }

    void InputProcessKey(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, BlitKey key, bool bPressed)
    {
        uint32_t idx = uint32_t(key);
        BlitEventType event{ BlitEventType::MaxTypes };
        auto& controller = SYSTEM->m_controllers[SYSTEM->m_activeControllerIDX];
        switch (controller.m_keyData[idx].m_keyCallbackType)
        {
        case KeyCallbackType::PRESS:
        {
            if (bPressed)
            {
                event = controller.KEYPRESS(idx, SYSTEM->pWORLD->deltaTime);
            }
            break;
        }
        case KeyCallbackType::RELEASE:
        {
            if (!bPressed)
            {
                event = controller.KEYRELEASE(idx, SYSTEM->pWORLD->deltaTime);
            }
            break;
        }
        case KeyCallbackType::HOLD:
        {
            if (bPressed)
            {
				controller.KEYHOLD(idx, SYSTEM->pWORLD->deltaTime);
            }
            else
            {
				controller.KEYRELEASEHOLD(idx, SYSTEM->pWORLD->deltaTime);
            }
            break;
        }
        case KeyCallbackType::HOLD_AND_RELEASE:
        {
            if (bPressed)
            {
                controller.KEYHOLD(idx, SYSTEM->pWORLD->deltaTime);
            }
            else
            {
                controller.KEYRELEASEHOLDPFN(idx, SYSTEM->pWORLD->deltaTime);
            }
            break;
        }
        default:
        {
            break;
        }
        }
        DispatchEventCallback(SYSTEM, event);
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
                SYSTEM->m_controllers[SYSTEM->m_activeControllerIDX].MBPRESS(idx, mouseX, mouseY, SYSTEM->pWORLD->deltaTime);
            }
            else
            {
                SYSTEM->m_controllers[SYSTEM->m_activeControllerIDX].MBPRESS(idx, mouseX, mouseY, SYSTEM->pWORLD->deltaTime);
            }
        }
    }

    void InputProcessMouseWheel(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, int8_t zDelta)
    {
        SYSTEM->m_controllers[SYSTEM->m_activeControllerIDX].WHEEL(zDelta, SYSTEM->pWORLD->deltaTime);
    }

    void RegisterKeyEvent(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, BlitKey key, KeyCallback callback, uint32_t controllerIDX, KeyCallbackType type)
    {
        auto& controller = SYSTEM->m_controllers[controllerIDX];
        auto& keyData = controller.m_keyData[uint32_t(key)];
        keyData.m_keyCallbackType = type;

        switch (type)
        {
            case KeyCallbackType::PRESS:
            case KeyCallbackType::RELEASE:
            {
                keyData.m_PFNTap = callback;
                break;
			}
            case KeyCallbackType::HOLD:
            {
                keyData.m_PFNHeld = callback;
				controller.m_keyHeldIdxs[controller.m_registeredKeyHeldCount++] = uint32_t(key);
                break;
            }
            default:
            {
                break;
            }
        }
    }

    void RegisterHoldReleaseKeyEvent(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, BlitKey key, KeyCallback holdCallback, KeyCallback tapCallback, uint32_t controllerIDX)
    {
        auto& keyData = SYSTEM->m_controllers[controllerIDX].m_keyData[uint32_t(key)];
        auto& controller = SYSTEM->m_controllers[controllerIDX];

        keyData.m_PFNHeld = holdCallback;
        keyData.m_PFNTap = tapCallback;
        keyData.m_keyCallbackType = KeyCallbackType::HOLD_AND_RELEASE;
        controller.m_keyHeldIdxs[controller.m_registeredKeyHeldCount++] = uint32_t(key);
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

    void DispatchRawInput_MOUSE_MOVED(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, int32_t xAxisMovement, int32_t yAxisMovement)
    {
        SYSTEM->m_controllers[SYSTEM->m_activeControllerIDX].MOUSEMOVE(xAxisMovement, yAxisMovement, SYSTEM->pWORLD->deltaTime);
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

    static BlitEventType CloseOnEscapeKeyPressCallback(BlitzenEngine::Resident resident, float deltaTime)
    {
		return BlitEventType::EngineShutdown;
    }

    static BlitEventType ForwardTestCallback(BlitzenEngine::Resident resident, float deltaTime)
    {
        BlitzenEngine::AddResidentVelocityZAxis(resident, deltaTime);
        //BlitzenWorld::RequestGameCameraRotation(resident, 0, 1);

        return BlitEventType::MaxTypes;
    }

    static BlitEventType BackwardTestCallback(BlitzenEngine::Resident resident, float deltaTime)
    {
        BlitzenEngine::AddResidentVelocityZAxisNegative(resident, deltaTime);
        //BlitzenWorld::RequestGameCameraRotation(resident, 0, -1);

        return BlitEventType::MaxTypes;
    }

    static BlitEventType StopMovingZAxisTestCallback(BlitzenEngine::Resident resident, float deltaTime)
    {
        BlitzenEngine::KillResidentVelocityZAxis(resident);

        return BlitEventType::MaxTypes;
    }

    static BlitEventType LeftTestCallback(BlitzenEngine::Resident resident, float deltaTime)
    {
        BlitzenEngine::AddResidentVelocityXAxisNegative(resident, deltaTime);
        //BlitzenWorld::RequestGameCameraRotation(resident, -1, 0);

        return BlitEventType::MaxTypes;
    }

    static BlitEventType RightTestCallback(BlitzenEngine::Resident resident, float deltaTime)
    {
        BlitzenEngine::AddResidentVelocityXAxis(resident, deltaTime);
        //BlitzenWorld::RequestGameCameraRotation(resident, 1, 0);

        return BlitEventType::MaxTypes;
    }

    static BlitEventType StopMovingXAxisTestCallback(BlitzenEngine::Resident resident, float deltaTime)
    {
        BlitzenEngine::KillResidentVelocityXAxis(resident);

        return BlitEventType::MaxTypes;
    }

    static BlitEventType ForwardMoveEngineCamera(BlitzenEngine::Resident resident, float deltaTime)
    {
        BlitzenWorld::MoveCameraReleased(BlitML::fVelocity{ 0.f, 0.f, 1.f });

        return BlitEventType::MaxTypes;
	}

    static BlitEventType BackwardMoveEngineCamera(BlitzenEngine::Resident resident, float deltaTime)
    {
        BlitzenWorld::MoveCameraReleased(BlitML::fVelocity{ 0.f, 0.f, -1.f });

        return BlitEventType::MaxTypes;
	}

    static BlitEventType LeftMoveEngineCamera(BlitzenEngine::Resident resident, float deltaTime)
    {
        BlitzenWorld::MoveCameraReleased(BlitML::fVelocity{ -1.f, 0.f, 0.f });

        return BlitEventType::MaxTypes;
    }

    static BlitEventType RightMoveEngineCamera(BlitzenEngine::Resident resident, float deltaTime)
    {
        BlitzenWorld::MoveCameraReleased(BlitML::fVelocity{ 1.f, 0.f, 0.f });

        return BlitEventType::MaxTypes;
	}

    static BlitEventType FreezeFrustumOnF1KeyPressCallback(BlitzenEngine::Resident resident, float deltaTime)
    {
        return BlitEventType::FreezeFrustum;
    }

    static BlitEventType ChangePyramidLevelOnF3ReleaseCallback(BlitzenEngine::Resident resident, float deltaTime)
    {
        return BlitEventType::HI_Z_MAP_levelIncrease;
    }

    static BlitEventType DecreasePyramidLevelOnF4ReleaseCallback(BlitzenEngine::Resident resident, float deltaTime)
    {
        return BlitEventType::HI_Z_MAP_levelDescrease;
    }

    static BlitEventType BringBackEditorOnF10(BlitzenEngine::Resident resident, float deltaTime)
    {
        return BlitEventType::BringBackEditor;
    }

    static BlitEventType BringDasherRuntimeDebugWindowOnF9(BlitzenEngine::Resident, float deltaTime)
    {
        return BlitEventType::BringDasherRuntimeDebugWindow;
    }

    static uint8_t ResizeEventCallback(BLIT_STRAIGHTHANDLE sysHandle, BlitzenCore::BlitEventType eventType)
    {
		auto SYSTEM{ reinterpret_cast<BlitzenWorld::BLITZEN_SYSTEM_CONTEXT*>(sysHandle) };
        auto pWORLD = reinterpret_cast<BlitzenWorld::WORLD_blit*>(SYSTEM->pWORLD);
        auto& camera{ pWORLD->m_cameras[pWORLD->m_activeCameraIDX]};

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

        BlitML::vec2 hizExtent{ BlitzenEngine::UpdateRendererWindowData(SYSTEM->pWORLD->BMPR.Data(), SYSTEM->WIDTH, SYSTEM->HEIGHT , SYSTEM->pPlatform)};

#if defined(DASHER_JOIN)
        context.pDasher->UpdateWindowSize(context.WIDTH, context.HEIGHT);
#endif

        camera.viewData.pyramidWidth = hizExtent.x;
        camera.viewData.pyramidHeight = hizExtent.y;

        return 1;
    }

    static BlitEventType OnMouseMove(BlitzenEngine::Resident resident, float deltaTime, int32_t xAxisMovement, int32_t yAxisMovement)
    {
        BlitzenWorld::RequestGameCameraRotation(resident, xAxisMovement, yAxisMovement);
        //BlitzenEngine::AddResidentVelocityXAxis(resident, deltaTime);
        return BlitEventType::MaxTypes;
    }

    static BlitEventType OnMouseButtonClickTest(BlitzenEngine::Resident resident, float deltaTime, int16_t mouseX, int16_t mouseY)
    {
        return BlitEventType::MaxTypes;
    }

    static BlitEventType OnMouseButtonReleaseTest(BlitzenEngine::Resident resident, float deltaTime, int16_t mouseX, int16_t mouseY)
    {
        return BlitEventType::MaxTypes;
    }

    static BlitEventType SnapToMainCharacter(BlitzenEngine::Resident resident, float deltaTime)
    {
        BlitzenWorld::SNAP_MAIN();

        return BlitEventType::MaxTypes;
    }

    void RegisterDefaultEvents(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM)
    {
        BlitzenCore::RegisterEvent(SYSTEM, BlitzenCore::BlitEventType::EngineShutdown, OnShutdown);

        BlitzenCore::RegisterEvent(SYSTEM, BlitzenCore::BlitEventType::WindowUpdate, ResizeEventCallback);

        BlitzenCore::RegisterKeyEvent(SYSTEM, BlitzenCore::BlitKey::__ESCAPE, CloseOnEscapeKeyPressCallback, BlitzenCore::CE_INITIAL_CONTROLLER_ID, KeyCallbackType::RELEASE);

        BlitzenCore::RegisterKeyEvent(SYSTEM, BlitzenCore::BlitKey::__ESCAPE, BringBackEditorOnF10, BlitzenCore::CE_INITIAL_CONTROLLER_ID, KeyCallbackType::RELEASE);

        BlitzenCore::RegisterMouseMoveCallback(SYSTEM, OnMouseMove, BlitzenCore::CE_INITIAL_CONTROLLER_ID);
        BlitzenCore::RegisterMouseMoveCallback(SYSTEM, OnMouseMove, 1);

        BlitzenCore::RegisterKeyEvent(SYSTEM, BlitzenCore::BlitKey::__W, ForwardMoveEngineCamera, BlitzenCore::CE_INITIAL_CONTROLLER_ID, KeyCallbackType::HOLD);
        BlitzenCore::RegisterHoldReleaseKeyEvent(SYSTEM, BlitzenCore::BlitKey::__W, ForwardTestCallback, StopMovingZAxisTestCallback, 1);

        BlitzenCore::RegisterKeyEvent(SYSTEM, BlitzenCore::BlitKey::__S, BackwardMoveEngineCamera, BlitzenCore::CE_INITIAL_CONTROLLER_ID, KeyCallbackType::HOLD);
        BlitzenCore::RegisterHoldReleaseKeyEvent(SYSTEM, BlitzenCore::BlitKey::__S, BackwardTestCallback, StopMovingZAxisTestCallback, 1);

        BlitzenCore::RegisterKeyEvent(SYSTEM, BlitzenCore::BlitKey::__A, LeftMoveEngineCamera, BlitzenCore::CE_INITIAL_CONTROLLER_ID, KeyCallbackType::HOLD);
        BlitzenCore::RegisterHoldReleaseKeyEvent(SYSTEM, BlitzenCore::BlitKey::__A, LeftTestCallback, StopMovingXAxisTestCallback, 1);

        BlitzenCore::RegisterKeyEvent(SYSTEM, BlitzenCore::BlitKey::__D, RightMoveEngineCamera, BlitzenCore::CE_INITIAL_CONTROLLER_ID, KeyCallbackType::HOLD);
        BlitzenCore::RegisterHoldReleaseKeyEvent(SYSTEM, BlitzenCore::BlitKey::__D, RightTestCallback, StopMovingXAxisTestCallback, 1);

        BlitzenCore::RegisterKeyEvent(SYSTEM, BlitzenCore::BlitKey::__TAB, SnapToMainCharacter, 1, KeyCallbackType::PRESS);

        BlitzenCore::RegisterMouseButtonPressAndReleaseCallback(SYSTEM, BlitzenCore::MouseButton::Left, OnMouseButtonClickTest, OnMouseButtonReleaseTest, BlitzenCore::CE_INITIAL_CONTROLLER_ID);

        BlitzenCore::RegisterKeyEvent(SYSTEM, BlitzenCore::BlitKey::__F8, BringDasherRuntimeDebugWindowOnF9, BlitzenCore::CE_INITIAL_CONTROLLER_ID, KeyCallbackType::PRESS);

        BlitzenCore::RegisterKeyEvent(SYSTEM, BlitzenCore::BlitKey::__F10, BringBackEditorOnF10, BlitzenCore::CE_INITIAL_CONTROLLER_ID, KeyCallbackType::PRESS);

#if !defined(BLIT_VK_FORCE)

        BlitzenCore::RegisterKeyEvent(SYSTEM, BlitzenCore::BlitKey::__F1, FreezeFrustumOnF1KeyPressCallback, BlitzenCore::CE_INITIAL_CONTROLLER_ID, KeyCallbackType::PRESS);
        BlitzenCore::RegisterKeyEvent(SYSTEM, BlitzenCore::BlitKey::__F1, FreezeFrustumOnF1KeyPressCallback, 1, KeyCallbackType::PRESS);

        BlitzenCore::RegisterKeyEvent(SYSTEM, BlitzenCore::BlitKey::__F3, ChangePyramidLevelOnF3ReleaseCallback, BlitzenCore::CE_INITIAL_CONTROLLER_ID, KeyCallbackType::PRESS);

        BlitzenCore::RegisterKeyEvent(SYSTEM, BlitzenCore::BlitKey::__F4, DecreasePyramidLevelOnF4ReleaseCallback, BlitzenCore::CE_INITIAL_CONTROLLER_ID, KeyCallbackType::PRESS);

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