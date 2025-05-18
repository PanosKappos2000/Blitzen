#pragma once
#include "blitEvents.h"
#include "blitMemory.h"
#include "blitKeys.h"

namespace BlitzenCore
{
    using KeyPressCallback = BlitCL::Pfn<void, void**>;
    using KeyReleaseCallback = BlitCL::Pfn<void, void**>;

    using MouseButtonPressCallback = BlitCL::Pfn<void, void**, int16_t, int16_t>;
    using MouseButtonReleaseCallback = BlitCL::Pfn<void, void**, int16_t, int16_t>;

    using MouseMoveCallbackType = BlitCL::Pfn<void, void**, int16_t, int16_t, int16_t, int16_t>;
    using MouseWheelCallbackType = BlitCL::Pfn<void, void**, int8_t>;

    // Mouse buttons and mouse position
    struct MouseState
    {
        int16_t x;
        int16_t y;
        bool buttons[uint8_t(MouseButton::MaxButtons)];
    };

    class InputSystemState 
    {
    public:
        
        KeyPressCallback keyPressCallbacks[Ce_KeyCallbackCount];
        KeyReleaseCallback keyReleaseCallbacks[Ce_KeyCallbackCount];

        MouseMoveCallbackType mouseMoveCallback;
        MouseWheelCallbackType mouseWheelCallback;

        MouseButtonReleaseCallback mouseReleaseCallbacks[3];
        MouseButtonPressCallback mousePressCallbacks[3];

    public:

        InputSystemState(void** pp); 

        inline ~InputSystemState() {}

        inline void UpdateInput(double deltaTime) 
        {
            BlitzenCore::BlitMemCopy(m_previousKeyboard, m_currentKeyboard, sizeof(m_currentKeyboard));
        }

        void InputProcessKey(BlitKey key, bool bPressed);

        void InputProcessButton(MouseButton button, bool bPressed);

        void InputProcessMouseMove(int16_t x, int16_t y);

        void InputProcessMouseWheel(int8_t zDelta);

        void** ppContext{ nullptr };

        bool m_currentKeyboard[Ce_KeyCallbackCount];
        bool m_previousKeyboard[Ce_KeyCallbackCount];

        MouseState m_currentMouse;
        MouseState m_previousMouse;
    };

    // Passes the logic to be called when a speicific key is pressed
    void RegisterKeyPressCallback(InputSystemState* pContext, BlitKey key, KeyPressCallback callback);

    // Passes the logic to be called when a specific key is released
    void RegisterKeyReleaseCallback(InputSystemState* pContext, BlitKey key, KeyReleaseCallback callback);

    // Passes logic for both key press and release
    void RegisterKeyPressAndReleaseCallback(InputSystemState* pContext, BlitKey key, KeyPressCallback press, KeyReleaseCallback release);

    // Passed logic to be called when one of the mouse buttons is pressed
    void RegisterMouseButtonPressCallback(InputSystemState* pContext, MouseButton button, MouseButtonPressCallback callback);

    // Initializes the release function pointer for the given key
    void RegisterMouseButtonReleaseCallback(InputSystemState* pContext, MouseButton button, MouseButtonReleaseCallback callback);

    // Initializes both the release and press function pointers for the given key
    void RegisterMouseButtonPressAndReleaseCallback(InputSystemState* pContext, MouseButton button, MouseButtonPressCallback press, MouseButtonReleaseCallback release);

    void RegisterMouseWheelCallback(InputSystemState* pContext, MouseWheelCallbackType callback);

    void RegisterMouseMoveCallback(InputSystemState* pContext, MouseMoveCallbackType callback);
}