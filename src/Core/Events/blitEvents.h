#pragma once
#include "Core/BlitzenWorld/blitzenWorldPrivate.h"

namespace BlitzenCore
{
    void ZeroInitializeEventFunctionPointers(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM);

    // Passes the logic to be called when a speicific key is pressed
    void RegisterKeyPressCallback(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, BlitKey key, KeyPressCallback callback, uint32_t controllerIDX);

    // Passes the logic to be called when a specific key is released
    void RegisterKeyReleaseCallback(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, BlitKey key, KeyReleaseCallback callback, uint32_t cotrollerIDX);

    // Passes logic for both key press and release
    void RegisterKeyPressAndReleaseCallback(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, BlitKey key, KeyPressCallback press, KeyReleaseCallback release, uint32_t controllerIDX);

    // Passed logic to be called when one of the mouse buttons is pressed
    void RegisterMouseButtonPressCallback(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, MouseButton button, MouseButtonPressCallback callback, uint32_t cotrollerIDX);

    // Initializes the release function pointer for the given key
    void RegisterMouseButtonReleaseCallback(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, MouseButton button, MouseButtonReleaseCallback callback, uint32_t cotrollerIDX);

    // Initializes both the release and press function pointers for the given key
    void RegisterMouseButtonPressAndReleaseCallback(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, MouseButton button, MouseButtonPressCallback press, MouseButtonReleaseCallback release, uint32_t controllerIDX);

    void RegisterMouseWheelCallback(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, MouseWheelCallbackType callback, uint32_t cotrollerIDX);

    void RegisterMouseMoveCallback(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, MouseMoveCallbackType callback, uint32_t cotrollerIDX);

    // Adds a new RegisteredEvent to the eventState event types array
    void RegisterEvent(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, BlitEventType type, BlitzenWorld::EventCallback eventCallback);

    void RegisterDefaultEvents(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM);

    void AssignEditorCallbacks(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM);

    void DispatchRawInput_MOUSE_MOVED(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, int16_t xAxisMovement, int16_t yAxisMovement);

    bool DispatchEventCallback(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, BlitEventType type);

    void UpdateInput(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, double deltaTime, EditorEventContext* pEditor = nullptr);

    void InputProcessKey(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, BlitKey key, BlitzenCore::FAT_BOOL bPressed);

    void InputProcessButton(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, MouseButton button, BlitzenCore::FAT_BOOL bPressed);

    void InputProcessMouseWheel(BlitzenWorld::BLITZEN_SYSTEM_CONTEXT* SYSTEM, int8_t zDelta);
}