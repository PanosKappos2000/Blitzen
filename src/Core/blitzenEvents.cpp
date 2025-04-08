#include "blitInput.h"
#include "Engine/blitzenEngine.h"

namespace BlitzenCore
{
    void RegisterEvent(BlitEventType type, void* pListener, EventCallbackType eventCallback)
    {
        if (type == BlitEventType::KeyPressed || type == BlitEventType::KeyReleased || 
            type == BlitEventType::MouseButtonPressed || type == BlitEventType::MouseButtonReleased)
            return;

        auto pEvents = EventSystemState::GetState()->eventTypes;
        pEvents[size_t(type)] = { pListener, eventCallback };
    }

    void RegisterKeyPressCallback(BlitKey key, BlitCL::Pfn<void> callback) 
    {
        InputSystemState::GetState()->keyPressCallbacks[size_t(key)] = callback;
    }
    
    void RegisterKeyReleaseCallback(BlitKey key, BlitCL::Pfn<void> callback)
    {
        InputSystemState::GetState()->keyReleaseCallbacks[size_t(key)] = callback;
    }

    void RegisterKeyPressAndReleaseCallback(BlitKey key, BlitCL::Pfn<void> press, BlitCL::Pfn<void> release
    )
    {
        InputSystemState::GetState()->keyPressCallbacks[size_t(key)] = press;
        InputSystemState::GetState()->keyReleaseCallbacks[size_t(key)] = release;
    }

    void CallKeyPressFunction(BlitKey key)
    {
        auto func = InputSystemState::GetState()->keyPressCallbacks[size_t(key)];
        if(func.IsFunctional())
            func();
    }

    void CallKeyReleaseFunction(BlitKey key)
    {
        auto func = InputSystemState::GetState()->keyReleaseCallbacks[size_t(key)];
        if(func.IsFunctional())
            func();
    }

    void InputProcessMouseMove(int16_t x, int16_t y) 
    {
        auto pState = InputSystemState::GetState();
		auto pEvents = EventSystemState::GetState();
        
        if (pState->currentMouse.x != x || pState->currentMouse.y != y) 
        {
            // TODO: Change this to something more general
            EventContext context;
            context.data.si16[0] = x - pState->currentMouse.x;
            context.data.si16[1] = y - pState->currentMouse.y;
            
            pState->currentMouse.x = x;
            pState->currentMouse.y = y;

            pEvents->FireEvent(BlitEventType::MouseMoved, nullptr, context);
        }
    }

    void InputProcessButton(MouseButton button, uint8_t bPressed)
    {
        auto pState = InputSystemState::GetState();

        // If the state changed, fire an event.
        if (pState->currentMouse.buttons[static_cast<size_t>(button)] != bPressed)
        {
            pState->currentMouse.buttons[static_cast<size_t>(button)] = bPressed;
            if (bPressed)
                CallMouseButtonPressFunction(button, pState->currentMouse.x, pState->currentMouse.y);
            else
                CallMouseButtonReleaseFunction(button, pState->currentMouse.x, pState->currentMouse.y);
           
        }
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

    void CallMouseButtonPressFunction(MouseButton button, int16_t mouseX, int16_t mouseY)
    {
        auto func = InputSystemState::GetState()->mousePressCallbacks[uint8_t(button)];
        if (func.IsFunctional())
            func(mouseX, mouseY);
    }

    void CallMouseButtonReleaseFunction(MouseButton button, int16_t mouseX, int16_t mouseY)
    {
        auto func = InputSystemState::GetState()->mouseReleaseCallbacks[uint8_t(button)];
        if (func.IsFunctional())
            func(mouseX, mouseY);
    }
    
    void InputProcessMouseWheel(int8_t zDelta) 
    {
        EventContext context;
        context.data.ui8[0] = zDelta;

        // this should not be an event, it should be an input
        BlitzenCore::EventSystemState::GetState()->FireEvent(BlitEventType::MouseWheel, nullptr, context);
    }

    uint8_t GetCurrentKeyState(BlitKey key) 
    {
        auto pState = InputSystemState::GetState();
        return pState->currentKeyboard[static_cast<size_t>(key)];
    }

    uint8_t GetPreviousKeyState(BlitKey key) 
    {
        auto pState = InputSystemState::GetState();
        return pState->currentKeyboard[static_cast<size_t>(key)];
    }

    uint8_t GetCurrentMouseButtonState(MouseButton button) 
    {
        auto pState = InputSystemState::GetState();
        return pState->currentMouse.buttons[static_cast<size_t>(button)];
    }

    uint8_t GetPreviousMouseButtonState(MouseButton button)
    {
        auto pState = InputSystemState::GetState();
        return pState->previousMouse.buttons[static_cast<size_t>(button)];
    }

    void GetMousePosition(int32_t* x, int32_t* y) 
    {
        auto pState = InputSystemState::GetState();
        *x = pState->previousMouse.x;
        *y = pState->currentMouse.y;
    }
    void GetPreviousMousePosition(int32_t* x, int32_t* y)
    {
        auto pState = InputSystemState::GetState();
        *x = pState->previousMouse.x;
        *y = pState->previousMouse.y;
    }
}