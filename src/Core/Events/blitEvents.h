#pragma once
#include "blitKeys.h"
#include "blitController.h"
#include "Core/BlitzenWorld/blitzenWorldPrivate.h"
#include "blitEditorEvents.h"

namespace BlitzenCore
{
    // Standard events
    using EventCallback = BlitCL::Pfn<uint8_t, BlitzenWorld::BLITZEN_SYSTEM_CONTEXT&, BlitEventType>;

    // Editor events take full context and return controller id
    using EditorCallback = BlitCL::Pfn<uint32_t, BlitzenWorld::BLITZEN_SYSTEM_CONTEXT&>;

    // Mouse buttons and mouse position
    struct MouseState
    {
        int16_t x;
        int16_t y;
        bool buttons[uint8_t(MouseButton::MaxButtons)];
    };

    class EventSystem 
    {
    public:
        
        EventSystem(BlitzenWorld::WORLD_blit* WORLD, BlitzenWorld::BLITZEN_SYSTEM_CONTEXT& systemContext);

        inline ~EventSystem() {}

        void UpdateInput(double deltaTime, EditorEventContext* pEditor = nullptr);

        void InputProcessKey(BlitKey key, bool bPressed);

        void InputProcessButton(MouseButton button, bool bPressed);

        void InputProcessMouseMove(int16_t x, int16_t y);

        void InputProcessMouseWheel(int8_t zDelta);

        bool FireEvent(BlitEventType type);

        BlitzenWorld::BLITZEN_SYSTEM_CONTEXT& m_systemContext;
        BlitzenWorld::WORLD_blit* m_pWorldContext;

        EventCallback m_eventCallbacks[uint8_t(BlitEventType::MaxTypes)]{};

        Controller m_controllers[Ce_MaxControllerCount];
        uint32_t m_activeControllerIDX{ Ce_InitialControllerID };

        EditorCallback m_editorButtonCallbacks[BlitzenCore::Ce_EditorButtonEventTypeCount]{ [](BlitzenWorld::BLITZEN_SYSTEM_CONTEXT&)->uint32_t {return Ce_InitialControllerID; } };

        bool m_currentKeyboard[Ce_KeyCallbackCount];
        bool m_previousKeyboard[Ce_KeyCallbackCount];

        MouseState m_currentMouse;
        MouseState m_previousMouse;
    };

    // Passes the logic to be called when a speicific key is pressed
    void RegisterKeyPressCallback(EventSystem* pContext, BlitKey key, KeyPressCallback callback, uint32_t controllerIDX);

    // Passes the logic to be called when a specific key is released
    void RegisterKeyReleaseCallback(EventSystem* pContext, BlitKey key, KeyReleaseCallback callback, uint32_t cotrollerIDX);

    // Passes logic for both key press and release
    void RegisterKeyPressAndReleaseCallback(EventSystem* pContext, BlitKey key, KeyPressCallback press, KeyReleaseCallback release, uint32_t controllerIDX);

    // Passed logic to be called when one of the mouse buttons is pressed
    void RegisterMouseButtonPressCallback(EventSystem* pContext, MouseButton button, MouseButtonPressCallback callback, uint32_t cotrollerIDX);

    // Initializes the release function pointer for the given key
    void RegisterMouseButtonReleaseCallback(EventSystem* pContext, MouseButton button, MouseButtonReleaseCallback callback, uint32_t cotrollerIDX);

    // Initializes both the release and press function pointers for the given key
    void RegisterMouseButtonPressAndReleaseCallback(EventSystem* pContext, MouseButton button, MouseButtonPressCallback press, MouseButtonReleaseCallback release, uint32_t controllerIDX);

    void RegisterMouseWheelCallback(EventSystem* pContext, MouseWheelCallbackType callback, uint32_t cotrollerIDX);

    void RegisterMouseMoveCallback(EventSystem* pContext, MouseMoveCallbackType callback, uint32_t cotrollerIDX);

    // Adds a new RegisteredEvent to the eventState event types array
    void RegisterEvent(EventSystem* pContext, BlitEventType type, EventCallback eventCallback);

    void RegisterDefaultEvents(EventSystem* pEvents);

    void AssignEditorCallbacks(EventSystem* pContext);

    using EventSystemMemory = BlitCL::SmartPointer<BlitzenCore::EventSystem>;
}