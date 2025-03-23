#include "blitEvents.h"

#include "Engine/blitzenEngine.h"

#define GET_EVENT_SYSTEM_STATE()    EventSystemState::GetState()

namespace BlitzenCore
{
    /*
        Event system
    */
    EventSystemState* EventSystemState::s_pEventSystemState = nullptr;

    EventSystemState::EventSystemState()
    {
        s_pEventSystemState = this;
    }

    EventSystemState::~EventSystemState()
    {
        s_pEventSystemState = nullptr;
    }



    /*
        Input System
    */
    InputSystemState* InputSystemState::s_pInputSystemState = nullptr;

    InputSystemState::InputSystemState() 
    {
        s_pInputSystemState = this;
    }

    InputSystemState::~InputSystemState() 
    {
        s_pInputSystemState = nullptr;
    }

    void UpdateInput(double deltaTime) 
    {
        InputSystemState* pState = InputSystemState::GetState();
        
        // Copy current states to previous states
        BlitzenCore::BlitMemCopy(&pState->previousKeyboard, &pState->currentKeyboard, 
            sizeof(pState->currentKeyboard)
        );
        BlitzenCore::BlitMemCopy(&pState->previousMouse, &pState->currentMouse, 
            sizeof(MouseState)
        );
    }



    void InputProcessMouseMove(int16_t x, int16_t y) 
    {
        InputSystemState* pState = InputSystemState::GetState();
        // Only process if actually different
        if (pState->currentMouse.x != x || pState->currentMouse.y != y) 
        {
            // The context holds the difference between the new and old mouse position, since that is more useful now
            EventContext context;
            context.data.si16[0] = x - pState->currentMouse.x;
            context.data.si16[1] = y - pState->currentMouse.y;
            
            pState->currentMouse.x = x;
            pState->currentMouse.y = y;

            FireEvent<void*>(BlitEventType::MouseMoved, nullptr, context);
        }
    }
    
    void InputProcessMouseWheel(int8_t zDelta) 
    {
        //No internal state to update, simply fires an event
        EventContext context;
        context.data.ui8[0] = zDelta;
        FireEvent<void*>(BlitEventType::MouseWheel, nullptr, context);
    }

    uint8_t GetCurrentKeyState(BlitKey key) 
    {
        InputSystemState* pState = InputSystemState::GetState();
        return pState->currentKeyboard[static_cast<size_t>(key)];
    }

    uint8_t GetPreviousKeyState(BlitKey key) 
    {
        InputSystemState* pState = InputSystemState::GetState();
        return pState->currentKeyboard[static_cast<size_t>(key)];
    }

    
    uint8_t GetCurrentMouseButtonState(MouseButton button) 
    {
        InputSystemState* pState = InputSystemState::GetState();
        return pState->currentMouse.buttons[static_cast<size_t>(button)];
    }

    uint8_t GetPreviousMouseButtonState(MouseButton button)
    {
        InputSystemState* pState = InputSystemState::GetState();
        return pState->previousMouse.buttons[static_cast<size_t>(button)];
    }

    void GetMousePosition(int32_t* x, int32_t* y) 
    {
        InputSystemState* pState = InputSystemState::GetState();
        *x = pState->previousMouse.x;
        *y = pState->currentMouse.y;
    }
    void GetPreviousMousePosition(int32_t* x, int32_t* y)
    {
        InputSystemState* pState = InputSystemState::GetState();
        *x = pState->previousMouse.x;
        *y = pState->previousMouse.y;
    }
}