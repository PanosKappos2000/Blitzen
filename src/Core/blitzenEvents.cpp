#include "blitEvents.h"

#include "mainEngine.h"

namespace BlitzenCore
{
    #define BLIT_ENGINE_SHUTDOWN_EXPECTED_EVENTS 1
    #define BLIT_KEY_PRESSED_EXPECTED_EVENTS 100
    #define BLIT_KEY_RELEASED_EXPECTED_EVENTS 100
    #define BLIT_MOUSE_BUTTON_PRESSED_EXPECTED_EVENTS 50
    #define BLIT_MOUSE_BUTTON_RELEASED_EXPECTED_EVENTS 50
    #define BLIT_MOUSE_MOVED_EXPECTED_EVENTS  10
    #define BLIT_MOUSE_WHEEL_EXPECTED_EVENTS  10
    #define BLIT_WINDOW_RESIZE_EXPECTED_EVENTS  10

    static uint32_t maxExpectedEvents[static_cast<size_t>(BlitEventType::MaxTypes)] = 
    {
        BLIT_ENGINE_SHUTDOWN_EXPECTED_EVENTS, 
        BLIT_KEY_PRESSED_EXPECTED_EVENTS, 
        BLIT_KEY_RELEASED_EXPECTED_EVENTS, 
        BLIT_MOUSE_BUTTON_PRESSED_EXPECTED_EVENTS,
        BLIT_MOUSE_BUTTON_RELEASED_EXPECTED_EVENTS,
        BLIT_MOUSE_MOVED_EXPECTED_EVENTS,
        BLIT_MOUSE_WHEEL_EXPECTED_EVENTS, 
        BLIT_WINDOW_RESIZE_EXPECTED_EVENTS
    };

    static EventSystemState* pEventState = nullptr;

    uint8_t EventsInit() 
    {
        if(!BlitzenEngine::Engine::GetEngineInstancePointer())
        {
            BLIT_ERROR("The event system cannot be initialized before Blitzen")
            return 0;
        }

        // Get a pointer to the event state, so that it can be accessed statically by functions in this file
        pEventState = &(BlitzenEngine::Engine::GetEngineInstancePointer()->GetEngineSystems().eventState);

        return 1;
    }

    void EventsShutdown() 
    {
        if(BlitzenEngine::Engine::GetEngineInstancePointer()->GetEngineSystems().eventSystem)
        {
            BLIT_ERROR("Blitzen has not given permission for events to shutdown")
            return;
        }
        // The EventSystemState array block should logically be freed on its own when the engine shuts down
    }

    uint8_t RegisterEvent(BlitEventType type, void* pListener, pfnOnEvent eventCallback) 
    {
        BlitCL::DynamicArray<RegisteredEvent>& events = pEventState->eventTypes[static_cast<size_t>(type)];
        // If no event of this type has been created before, reserve the maximum expected space for events of this type
        if(events.GetSize() == 0) 
        {
            events.Reserve(maxExpectedEvents[static_cast<size_t>(type)]);
        }

        for(size_t i = 0; i < events.GetSize(); ++i) 
        {
            // A listener should not have multiple event callbacks for the same type of event
            if(events[i].pListener == pListener) {
                // TODO: warn
                return 0;
            }
        }

        // If at this point, no duplicate was found. Proceed with registration.
        RegisteredEvent event;
        event.pListener = pListener;
        event.callback = eventCallback;
        events.PushBack(event);
        return 1;
    }

    uint8_t UnregisterEvent(BlitEventType type, void* pListener, pfnOnEvent eventCallback) 
    {
        BlitCL::DynamicArray<RegisteredEvent>& events = pEventState->eventTypes[static_cast<size_t>(type)];
        // On nothing is registered for the code, boot out.
        if(events.GetSize() == 0) 
        {
            // TODO: warn
            return 0;
        }

        for(size_t i = 0; i < events.GetSize(); ++i) 
        {
            // Find the listener and remove the function pointer
            if(events[i].pListener == pListener && events[i].callback == eventCallback) 
            {
                events.RemoveAtIndex(i);
                return 1;
            }
        }

        // Not found
        return 0;
    }
    
    uint8_t FireEvent(BlitEventType type, void* pSender, EventContext context)
    {
        BlitCL::DynamicArray<RegisteredEvent>& events = pEventState->eventTypes[static_cast<size_t>(type)];
        // If nothing is registered for the code, boot out.
        if(events.GetSize() == 0) 
        {
            // TODO: warn
            return 0;
        }
        
        for(size_t i = 0; i < events.GetSize(); ++i) {
            RegisteredEvent& event = events[i];
            if(event.callback && event.callback(type, pSender, event.pListener, context)) 
            {
                // Event handled
                return 1;
            }
        }

        return 0;
    }





    
    static InputState* s_pInputState = nullptr;

    void InputInit(InputState* pInputState) 
    {
        s_pInputState = pInputState;
        if(s_pInputState)
            BLIT_INFO("Input system initialized.")
        else
            BLIT_FATAL("Input system initialization failed")
    }

    void InputShutdown() 
    {
        if(BlitzenEngine::Engine::GetEngineInstancePointer()->GetEngineSystems().inputSystem)
        {
            BLIT_ERROR("Blitzen has not given permission for input to shutdown")
            return;
        }
        // TODO: Add shutdown routines when needed.
    }

    void UpdateInput(double delta_time) 
    {
        // Copy current states to previous states
        BlitzenCore::BlitMemCopy(&s_pInputState->previousKeyboard, &s_pInputState->currentKeyboard, sizeof(KeyboardState));
        BlitzenCore::BlitMemCopy(&s_pInputState->previousMouse, &s_pInputState->currentMouse, sizeof(MouseState));
    }

    void InputProcessKey(BlitKey key, uint8_t bPressed) 
    {
        // Check If the key has not already been flagged as the value of bPressed
        if (s_pInputState->currentKeyboard.keys[static_cast<size_t>(key)] != bPressed) 
        {
            // Change the state to bPressed
            s_pInputState->currentKeyboard.keys[static_cast<size_t>(key)] = bPressed;

            // Fire off an event for immediate processing after saving the data of the input to the event context
            EventContext context;
            context.data.ui16[0] = static_cast<uint16_t>(key);
            FireEvent(bPressed ? BlitEventType::KeyPressed : BlitEventType::KeyReleased, nullptr, context);
        }
    }

    void InputProcessButton(MouseButton button, uint8_t bPressed) 
    {
        // If the state changed, fire an event.
        if (s_pInputState->currentMouse.buttons[static_cast<size_t>(button)] != bPressed) 
        {
            s_pInputState->currentMouse.buttons[static_cast<size_t>(button)] = bPressed;
            // Fire the event.
            EventContext context;
            context.data.ui16[0] = static_cast<uint16_t>(button);
            FireEvent(bPressed ? BlitEventType::MouseButtonPressed : BlitEventType::MouseButtonPressed, nullptr, context);
        }
    }

    void InputProcessMouseMove(int16_t x, int16_t y) 
    {
        // Only process if actually different
        if (s_pInputState->currentMouse.x != x || s_pInputState->currentMouse.y != y) {
            
            s_pInputState->currentMouse.x = x;
            s_pInputState->currentMouse.y = y;

            // Fire the event
            EventContext context;
            context.data.ui16[0] = x;
            context.data.ui16[1] = y;
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
        return s_pInputState->currentKeyboard.keys[static_cast<size_t>(key)];
    }

    uint8_t GetPreviousKeyState(BlitKey key) 
    {
        return s_pInputState->currentKeyboard.keys[static_cast<size_t>(key)];
    }

    
    uint8_t GetCurrentMouseButtonState(MouseButton button) 
    {
        return s_pInputState->currentMouse.buttons[static_cast<size_t>(button)];
    }


    uint8_t GetPreviousMouseButtonState(MouseButton button)
    {
        return s_pInputState->previousMouse.buttons[static_cast<size_t>(button)];
    }

    void GetMousePosition(int32_t* x, int32_t* y) 
    {
        *x = s_pInputState->previousMouse.x;
        *y = s_pInputState->currentMouse.y;
    }
    void GetPreviousMousePosition(int32_t* x, int32_t* y)
    {
        *x = s_pInputState->previousMouse.x;
        *y = s_pInputState->previousMouse.y;
    }
}