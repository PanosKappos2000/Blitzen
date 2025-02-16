#include "blitEvents.h"

#include "Engine/blitzenEngine.h"

#define GET_EVENT_SYSTEM_STATE()    EventSystemState::GetState()
#define GET_INPUT_SYSTEM_STATE()    InputSystemState::GetState()

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
    
    uint8_t FireEvent(BlitEventType type, void* pSender, const EventContext& context)
    {
        RegisteredEvent event = EventSystemState::GetState()->eventTypes[static_cast<size_t>(type)];
        if(event.callback)
            return event.callback(type, pSender, event.pListener, context);

        return 0;
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
        InputSystemState* pState = GET_INPUT_SYSTEM_STATE();
        
        // Copy current states to previous states
        BlitzenCore::BlitMemCopy(&(pState->previousKeyboard), &pState->currentKeyboard, sizeof(pState->currentKeyboard));
        BlitzenCore::BlitMemCopy(&pState->previousMouse, &pState->currentMouse, sizeof(MouseState));
    }

    void InputProcessKey(BlitKey key, uint8_t bPressed) 
    {
        InputSystemState* pState = GET_INPUT_SYSTEM_STATE();
        // Check If the key has not already been flagged as the value of bPressed
        if (pState->currentKeyboard[static_cast<size_t>(key)] != bPressed) 
        {
            // Change the state to bPressed
            pState->currentKeyboard[static_cast<size_t>(key)] = bPressed;

            // Fire off an event for immediate processing after saving the data of the input to the event context
            EventContext context;
            context.data.ui16[0] = static_cast<uint16_t>(key);
            FireEvent(bPressed ? BlitEventType::KeyPressed : BlitEventType::KeyReleased, nullptr, context);
        }
    }

    void InputProcessButton(MouseButton button, uint8_t bPressed) 
    {
        InputSystemState* pState = GET_INPUT_SYSTEM_STATE();
        // If the state changed, fire an event.
        if (pState->currentMouse.buttons[static_cast<size_t>(button)] != bPressed) 
        {
            pState->currentMouse.buttons[static_cast<size_t>(button)] = bPressed;
            // Fire the event.
            EventContext context;
            context.data.ui16[0] = static_cast<uint16_t>(button);
            FireEvent(bPressed ? BlitEventType::MouseButtonPressed : BlitEventType::MouseButtonPressed, nullptr, context);
        }
    }

    void InputProcessMouseMove(int16_t x, int16_t y) 
    {
        InputSystemState* pState = GET_INPUT_SYSTEM_STATE();
        // Only process if actually different
        if (pState->currentMouse.x != x || pState->currentMouse.y != y) 
        {
            // The context holds the difference between the new and old mouse position, since that is more useful now
            EventContext context;
            context.data.si16[0] = x - pState->currentMouse.x;
            context.data.si16[1] = y - pState->currentMouse.y;
            
            pState->currentMouse.x = x;
            pState->currentMouse.y = y;

            FireEvent(BlitEventType::MouseMoved, nullptr, context);
        }
    }
    
    void InputProcessMouseWheel(int8_t zDelta) 
    {
        //No internal state to update, simply fires an event
        EventContext context;
        context.data.ui8[0] = zDelta;
        FireEvent(BlitEventType::MouseWheel, nullptr, context);
    }

    uint8_t GetCurrentKeyState(BlitKey key) 
    {
        InputSystemState* pState = GET_INPUT_SYSTEM_STATE();
        return pState->currentKeyboard[static_cast<size_t>(key)];
    }

    uint8_t GetPreviousKeyState(BlitKey key) 
    {
        InputSystemState* pState = GET_INPUT_SYSTEM_STATE();
        return pState->currentKeyboard[static_cast<size_t>(key)];
    }

    
    uint8_t GetCurrentMouseButtonState(MouseButton button) 
    {
        InputSystemState* pState = GET_INPUT_SYSTEM_STATE();
        return pState->currentMouse.buttons[static_cast<size_t>(button)];
    }

    uint8_t GetPreviousMouseButtonState(MouseButton button)
    {
        InputSystemState* pState = GET_INPUT_SYSTEM_STATE();
        return pState->previousMouse.buttons[static_cast<size_t>(button)];
    }

    void GetMousePosition(int32_t* x, int32_t* y) 
    {
        InputSystemState* pState = GET_INPUT_SYSTEM_STATE();
        *x = pState->previousMouse.x;
        *y = pState->currentMouse.y;
    }
    void GetPreviousMousePosition(int32_t* x, int32_t* y)
    {
        InputSystemState* pState = GET_INPUT_SYSTEM_STATE();
        *x = pState->previousMouse.x;
        *y = pState->previousMouse.y;
    }
}