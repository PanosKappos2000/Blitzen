#include "blitInput.h"
#include "Engine/blitzenEngine.h"

namespace BlitzenCore
{
    void RegisterEvent(BlitEventType type, void* pListener, EventCallbackType eventCallback)
    {
        if (type == BlitEventType::KeyPressed
            || type == BlitEventType::KeyReleased
            || type == BlitEventType::MouseButtonPressed
            || type == BlitEventType::MouseButtonReleased
            || type == BlitEventType::MouseMoved
            || type == BlitEventType::MouseWheel
            )
        {
            return;
        }

        auto pEvents = EventSystemState::GetState()->eventTypes;
        pEvents[size_t(type)] = { pListener, eventCallback };
    }

    InputSystemState::InputSystemState()
    {
        for (uint32_t i = 0; i < KeyCallbackCount; ++i)
        {
            keyPressCallbacks[i] = []() {BLIT_INFO("No callback assigned"); };
            keyReleaseCallbacks[i] = []() {};
        }

        for (uint32_t i = 0; i < uint8_t(MouseButton::MaxButtons); i++)
        {
            mousePressCallbacks[i] = [](int16_t, int16_t) {};
            mouseReleaseCallbacks[i] = [](int16_t, int16_t) {};
        }

        mouseMoveCallback = [](int16_t, int16_t, int16_t, int16_t) {};

        mouseWheelCallback = [](int8_t) {};

        s_pInputSystemState = this;
    }

    void InputSystemState::InputProcessKey(BlitKey key, bool bPressed)
    {
        auto idx = static_cast<uint16_t>(key);
        if (currentKeyboard[idx] != bPressed)
        {
            currentKeyboard[idx] = bPressed;
            if (bPressed)
                keyPressCallbacks[idx]();
            else
                keyReleaseCallbacks[idx]();
        }
    }

    void InputSystemState::InputProcessMouseMove(int16_t x, int16_t y)
    {
        auto pState = InputSystemState::GetState();
        auto pEvents = EventSystemState::GetState();

        if (currentMouse.x != x || currentMouse.y != y)
        {
            previousMouse.x = currentMouse.x;
            previousMouse.y = currentMouse.y;
            currentMouse.x = x;
            currentMouse.y = y;

            mouseMoveCallback(currentMouse.x, currentMouse.y, previousMouse.x, previousMouse.y);
        }
    }

    void InputSystemState::InputProcessButton(MouseButton button, bool bPressed)
    {
        auto idx = static_cast<uint8_t>(button);
        // If the state changed, fire an event.
        if (currentMouse.buttons[idx] != bPressed)
        {
            currentMouse.buttons[idx] = bPressed;
            if (bPressed)
                mousePressCallbacks[idx](currentMouse.x, currentMouse.y);
            else
                mouseReleaseCallbacks[idx](currentMouse.x, currentMouse.y);
        }
    }

    void InputSystemState::InputProcessMouseWheel(int8_t zDelta)
    {
        mouseWheelCallback(zDelta);
    }

    void RegisterKeyPressCallback(BlitKey key, BlitCL::Pfn<void> callback)
    {
        InputSystemState::GetState()->keyPressCallbacks[size_t(key)] = callback;
    }

    void RegisterKeyReleaseCallback(BlitKey key, BlitCL::Pfn<void> callback)
    {
        InputSystemState::GetState()->keyReleaseCallbacks[size_t(key)] = callback;
    }

    void RegisterKeyPressAndReleaseCallback(BlitKey key, BlitCL::Pfn<void> press,
        BlitCL::Pfn<void> release)
    {
        InputSystemState::GetState()->keyPressCallbacks[size_t(key)] = press;
        InputSystemState::GetState()->keyReleaseCallbacks[size_t(key)] = release;
    }

    void RegisterMouseCallback(MouseMoveCallbackType callback)
    {
        InputSystemState::GetState()->mouseMoveCallback = callback;
    }

    void RegisterMouseButtonPressCallback(MouseButton button, BlitCL::Pfn<void, int16_t, int16_t> callback)
    {
        InputSystemState::GetState()->mousePressCallbacks[uint8_t(button)] = callback;
    }

    void RegisterMouseButtonReleaseCallback(MouseButton button, BlitCL::Pfn<void, int16_t, int16_t> callback)
    {
        InputSystemState::GetState()->mouseReleaseCallbacks[uint8_t(button)] = callback;
    }

    void RegisterMouseButtonPressAndReleaseCallback(MouseButton button,
        BlitCL::Pfn<void, int16_t, int16_t> press, BlitCL::Pfn<void, int16_t, int16_t> release
    )
    {
        InputSystemState::GetState()->mousePressCallbacks[uint8_t(button)] = press;
        InputSystemState::GetState()->mouseReleaseCallbacks[uint8_t(button)] = release;
    }

    void RegisterMouseWheelCallback(MouseWheelCallbackType callback)
    {
        InputSystemState::GetState()->mouseWheelCallback = callback;
    }
}