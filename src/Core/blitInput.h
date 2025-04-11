#pragma once
#include "blitEvents.h"
#include "blitMemory.h"
#include "blitKeys.h"

namespace BlitzenCore
{
    using MouseMoveCallbackType = BlitCL::Pfn<void, int16_t, int16_t, int16_t, int16_t>;
    using MouseWheelCallbackType = BlitCL::Pfn<void, int8_t>;

    // Passes the logic to be called when a speicific key is pressed
    void RegisterKeyPressCallback(BlitKey key, BlitCL::Pfn<void> callback); 
    // Passes the logic to be called when a specific key is released
    void RegisterKeyReleaseCallback(BlitKey key, BlitCL::Pfn<void> callback);
    // Passes logic for both key press and release
    void RegisterKeyPressAndReleaseCallback(BlitKey key, BlitCL::Pfn<void> press, BlitCL::Pfn<void> release);
    // Passed logic to be called when one of the mouse buttons is pressed
    void RegisterMouseButtonPressCallback(MouseButton button, BlitCL::Pfn<void, int16_t, int16_t> callback);
    // Initializes the release function pointer for the given key
    void RegisterMouseButtonReleaseCallback(MouseButton button, BlitCL::Pfn<void, int16_t, int16_t> callback);
    // Initializes both the release and press function pointers for the given key
    void RegisterMouseButtonPressAndReleaseCallback(MouseButton button,
        BlitCL::Pfn<void, int16_t, int16_t> press, BlitCL::Pfn<void, int16_t, int16_t> release);
    void RegisterMouseCallback(MouseMoveCallbackType callback);
    void RegisterMouseWheelCallback(MouseWheelCallbackType callback);

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
        // Each key has a pair of callbacks for press and release
        BlitCL::Pfn<void> keyPressCallbacks[KeyCallbackCount];

        BlitCL::Pfn<void> keyReleaseCallbacks[KeyCallbackCount];

        bool currentKeyboard[KeyCallbackCount];
        bool previousKeyboard[KeyCallbackCount];

        MouseState currentMouse;
        MouseState previousMouse;

        MouseMoveCallbackType mouseMoveCallback;
        MouseWheelCallbackType mouseWheelCallback;

        BlitCL::Pfn<void, int16_t, int16_t> mouseReleaseCallbacks[3];
        BlitCL::Pfn<void, int16_t, int16_t> mousePressCallbacks[3];

    public:

        InputSystemState(); 

        ~InputSystemState() { s_pInputSystemState = nullptr; }

        inline void UpdateInput(double deltaTime) 
        {
            BlitzenCore::BlitMemCopy(previousKeyboard, currentKeyboard, sizeof(currentKeyboard));
        }

        void InputProcessKey(BlitKey key, bool bPressed);

        void InputProcessButton(MouseButton button, bool bPressed);

        void InputProcessMouseMove(int16_t x, int16_t y);

        void InputProcessMouseWheel(int8_t zDelta);

        inline static InputSystemState* GetState() { return s_pInputSystemState; }
    private:
        
        inline static InputSystemState* s_pInputSystemState;
    };
}