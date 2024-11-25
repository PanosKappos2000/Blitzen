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

    // Because this will be placed in an array and arrays of arrays look ugly to me, I decided to use this
    // Also this should somehow be implemented with a hashmap at some point but I do not have a custom hashtable yet
    using EventTypeEntry =  BlitCL::DynamicArray<RegisteredEvent>;
    
    #define MAX_MESSAGE_CODES 16384

    struct EventSystemState 
    {
        EventTypeEntry eventTypes[MAX_MESSAGE_CODES];
    };

    static EventSystemState eventState;

    uint8_t EventsInit() 
    {
        BlitZeroMemory(&eventState, sizeof(eventState));
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
        EventTypeEntry& events = eventState.eventTypes[static_cast<size_t>(type)];
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
        EventTypeEntry& events = eventState.eventTypes[static_cast<size_t>(type)];
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
        EventTypeEntry& events = eventState.eventTypes[static_cast<size_t>(type)];
        // If nothing is registered for the code, boot out.
        if(events.GetSize() == 0) 
        {
            // TODO: warn
            return 0;
        }
        
        for(size_t i = 0; i < events.GetSize(); ++i) {
            RegisteredEvent& event = events[i];
            if(event.callback && event.callback(static_cast<size_t>(type), pSender, event.pListener, context)) 
            {
                // Event handled
                return 1;
            }
        }

        // Not found
        return 0;
    }




    struct KeyboardState 
    {
        uint8_t keys[256];
    };

    struct MouseState 
    {
        int16_t x;
        int16_t y;
        uint8_t buttons[static_cast<size_t>(MouseButton::MaxButtons)];
    };

    struct InputState 
    {
        KeyboardState currentKeyboard;
        KeyboardState previousKeyboard;
        MouseState currentMouse;
        MouseState previousMouse;
    };

    static InputState inputState = {};

    void InputInit() 
    {
        BLIT_INFO("Input system initialized.")
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
        // Copy current states to previous states.
        BlitzenCore::BlitMemCopy(&inputState.previousKeyboard, &inputState.currentKeyboard, sizeof(KeyboardState));
        BlitzenCore::BlitMemCopy(&inputState.previousMouse, &inputState.currentMouse, sizeof(MouseState));
    }

    void InputProcessKey(BlitKey key, uint8_t pressed) {
        
        if (inputState.currentKeyboard.keys[static_cast<size_t>(key)] != pressed) 
        {
            inputState.currentKeyboard.keys[static_cast<size_t>(key)] = pressed;

            // Fire off an event for immediate processing.
            EventContext context;
            context.data.ui16[0] = static_cast<uint16_t>(key);
            FireEvent(pressed ? BlitEventType::KeyPressed : BlitEventType::KeyReleased, nullptr, context);
        }
    }

    void InputProcessButton(MouseButton button, uint8_t pressed) 
    {
        // If the state changed, fire an event.
        if (inputState.currentMouse.buttons[static_cast<size_t>(button)] != pressed) 
        {
            inputState.currentMouse.buttons[static_cast<size_t>(button)] = pressed;
            // Fire the event.
            EventContext context;
            context.data.ui16[0] = static_cast<uint16_t>(button);
            FireEvent(pressed ? BlitEventType::MouseButtonPressed : BlitEventType::MouseButtonPressed, nullptr, context);
        }
    }

    void InputProcessMouseMove(int16_t x, int16_t y) 
    {
        // Only process if actually different
        if (inputState.currentMouse.x != x || inputState.currentMouse.y != y) {
            
            BLIT_DBLOG("Mouse moved-> x: %i, y: %i", inputState.currentMouse.x - x, inputState.currentMouse.y - y)
            
            inputState.currentMouse.x = x;
            inputState.currentMouse.y = y;

            // Fire the event
            EventContext context;
            context.data.ui16[0] = x;
            context.data.ui16[1] = y;
            FireEvent(BlitEventType::MouseMoved, nullptr, context);
        }
    }
    void InputProcessMouseWheel(int8_t z_delta) 
    {
        //No internal state to update, simply fires an event
        EventContext context;
        context.data.ui8[0] = z_delta;
        FireEvent(BlitEventType::MouseWheel, nullptr, context);
    }

    uint8_t GetCurrentKeyState(BlitKey key) 
    {
        return inputState.currentKeyboard.keys[static_cast<size_t>(key)];
    }

    uint8_t GetPreviousKeyState(BlitKey key) 
    {
        return inputState.currentKeyboard.keys[static_cast<size_t>(key)];
    }

    
    uint8_t GetCurrentMouseButtonState(MouseButton button) 
    {
        return inputState.currentMouse.buttons[static_cast<size_t>(button)];
    }


    uint8_t GetPreviousMouseButtonState(MouseButton button)
    {
        return inputState.previousMouse.buttons[static_cast<size_t>(button)];
    }

    void GetMousePosition(int32_t* x, int32_t* y) 
    {
        *x = inputState.previousMouse.x;
        *y = inputState.currentMouse.y;
    }
    void GetPreviousMousePosition(int32_t* x, int32_t* y)
    {
        *x = inputState.previousMouse.x;
        *y = inputState.previousMouse.y;
    }
}