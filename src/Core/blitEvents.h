#pragma once
#include "blitMemory.h"
#include "BlitCL/blitPfn.h"
#include "blitKeys.h"
#include "Core/BlitzenWorld/blitzenWorld.h"
#include "Core/BlitzenWorld/blitzenWorldPrivate.h"

namespace BlitzenCore
{
    enum class BlitEventType : uint8_t
    {
        EngineShutdown = 0,

        RendererTransformUpdate = 1,

        WindowUpdate = 2,

        MaxTypes = 8
    };

    using EventCallback = BlitCL::Pfn<uint8_t, BlitzenWorld::BlitzenPrivateContext&, BlitEventType>;

    using KeyPressCallback = BlitCL::Pfn<BlitEventType, BlitzenWorld::BlitzenWorldContext&>;
    using KeyReleaseCallback = BlitCL::Pfn<BlitEventType, BlitzenWorld::BlitzenWorldContext&>;

    using MouseButtonPressCallback = BlitCL::Pfn<BlitEventType, BlitzenWorld::BlitzenWorldContext&, int16_t, int16_t>;
    using MouseButtonReleaseCallback = BlitCL::Pfn<BlitEventType, BlitzenWorld::BlitzenWorldContext&, int16_t, int16_t>;

    using MouseMoveCallbackType = BlitCL::Pfn<BlitEventType, BlitzenWorld::BlitzenWorldContext&, int16_t, int16_t, int16_t, int16_t>;
    using MouseWheelCallbackType = BlitCL::Pfn<BlitEventType, BlitzenWorld::BlitzenWorldContext&, int8_t>;

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
        
        EventSystem(BlitzenWorld::BlitzenWorldContext& blitzenContext, BlitzenWorld::BlitzenPrivateContext& privateContext);

        inline ~EventSystem() {}

        inline void UpdateInput(double deltaTime) 
        {
            BlitzenCore::BlitMemCopy(m_previousKeyboard, m_currentKeyboard, sizeof(m_currentKeyboard));
        }

        void InputProcessKey(BlitKey key, bool bPressed);

        void InputProcessButton(MouseButton button, bool bPressed);

        void InputProcessMouseMove(int16_t x, int16_t y);

        void InputProcessMouseWheel(int8_t zDelta);

        bool FireEvent(BlitEventType type);

        BlitzenWorld::BlitzenPrivateContext& m_privateContext;
        BlitzenWorld::BlitzenWorldContext& m_blitzenContext;

        EventCallback m_eventCallbacks[uint8_t(BlitEventType::MaxTypes)];

        KeyPressCallback keyPressCallbacks[Ce_KeyCallbackCount];
        KeyReleaseCallback keyReleaseCallbacks[Ce_KeyCallbackCount];

        MouseMoveCallbackType mouseMoveCallback;
        MouseWheelCallbackType mouseWheelCallback;

        MouseButtonReleaseCallback mouseReleaseCallbacks[3];
        MouseButtonPressCallback mousePressCallbacks[3];

        bool m_currentKeyboard[Ce_KeyCallbackCount];
        bool m_previousKeyboard[Ce_KeyCallbackCount];

        MouseState m_currentMouse;
        MouseState m_previousMouse;
    };

    // Passes the logic to be called when a speicific key is pressed
    void RegisterKeyPressCallback(EventSystem* pContext, BlitKey key, KeyPressCallback callback);

    // Passes the logic to be called when a specific key is released
    void RegisterKeyReleaseCallback(EventSystem* pContext, BlitKey key, KeyReleaseCallback callback);

    // Passes logic for both key press and release
    void RegisterKeyPressAndReleaseCallback(EventSystem* pContext, BlitKey key, KeyPressCallback press, KeyReleaseCallback release);

    // Passed logic to be called when one of the mouse buttons is pressed
    void RegisterMouseButtonPressCallback(EventSystem* pContext, MouseButton button, MouseButtonPressCallback callback);

    // Initializes the release function pointer for the given key
    void RegisterMouseButtonReleaseCallback(EventSystem* pContext, MouseButton button, MouseButtonReleaseCallback callback);

    // Initializes both the release and press function pointers for the given key
    void RegisterMouseButtonPressAndReleaseCallback(EventSystem* pContext, MouseButton button, MouseButtonPressCallback press, MouseButtonReleaseCallback release);

    void RegisterMouseWheelCallback(EventSystem* pContext, MouseWheelCallbackType callback);

    void RegisterMouseMoveCallback(EventSystem* pContext, MouseMoveCallbackType callback);

    // Adds a new RegisteredEvent to the eventState event types array
    void RegisterEvent(EventSystem* pContext, BlitEventType type, EventCallback eventCallback);


    void RegisterDefaultEvents(EventSystem* pEvents);
}