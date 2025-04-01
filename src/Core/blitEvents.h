#pragma once

#include "blitzenContainerLibrary.h"

namespace BlitzenCore
{
    struct EventContext 
    {
        // 128 bytes
        union 
        {
            int64_t si64[2];
            uint64_t ui64[2];
            double f2[2];
            int32_t si32[4];
            uint32_t ui32[4];
            float f[4];
            int16_t si16[8];
            uint16_t ui16[8];
            int8_t si8[16];
            int8_t ui8[16];
            char c[16];
        } data;
    };

    enum class BlitEventType : uint16_t
    {
        EngineShutdown = 0,

        KeyPressed = 1,
        KeyReleased = 2,

        MouseButtonPressed = 3,
        MouseButtonReleased = 4,

        MouseMoved = 5,

        MouseWheel = 6,

        WindowResize = 7,

        MaxTypes = 8
    };

    using EventCallbackType = 
        BlitCL::Pfn<uint8_t, BlitEventType, void*, void*, const EventContext&>;
    struct RegisteredEvent 
    {
        void* pListener;
        EventCallbackType callback;
    };

    struct EventSystemState 
    {
        RegisteredEvent eventTypes[static_cast<uint8_t>(BlitEventType::MaxTypes)];

        EventSystemState();

        ~EventSystemState();

        static EventSystemState* s_pEventSystemState;

        inline static EventSystemState* GetState() { return s_pEventSystemState; }
    };

    // Adds a new RegisteredEvent to the eventState event types array
    inline void RegisterEvent(BlitEventType type, void* pListener, EventCallbackType eventCallback)
    {
		static uint8_t bSH;// Shutdown event already assigned ?

        // Some events do not have cusomizable callbacks
        if (type == BlitEventType::KeyPressed || type == BlitEventType::KeyReleased || 
            type == BlitEventType::MouseButtonPressed || type == BlitEventType::MouseButtonReleased)
            return;

        if(type == BlitEventType::EngineShutdown)
        {
            if(bSH)
                return;
            else
                bSH = 1;
        }
        
        auto pEvents = EventSystemState::GetState()->eventTypes;
        pEvents[size_t(type)] = {pListener, eventCallback};
    }

    inline uint8_t FireEvent(BlitEventType type, void* pSender, const EventContext& context)
    {
		if (type == BlitEventType::KeyPressed || type == BlitEventType::KeyReleased)
		{
			BLIT_ERROR("Use the InputProcessKey function to fire key events")
			return 0;
		}

        auto event = EventSystemState::GetState()->eventTypes[static_cast<uint8_t>(type)];
        if (event.callback.IsFunctional())
            return event.callback(type, pSender, event.pListener, context);

        return 0;
    }





    /*-------------------
        Input
    --------------------*/

    enum class MouseButton : uint8_t
    {
        Left,
        Right,
        Middle,
        MaxButtons
    };

    //This will create a custom define
    #define DEFINE_KEY(name, code) __##name = code

    enum class BlitKey 
    {
        //The key mappings are taken from windows
        DEFINE_KEY(BACKSPACE, 0x08),
        DEFINE_KEY(ENTER, 0x0D),
        DEFINE_KEY(TAB, 0x09),
        DEFINE_KEY(SHIFT, 0x10),
        DEFINE_KEY(CONTROL, 0x11),
        DEFINE_KEY(PAUSE, 0x13),
        DEFINE_KEY(CAPITAL, 0x14),
        DEFINE_KEY(ESCAPE, 0x1B),
        DEFINE_KEY(CONVERT, 0x1C),
        DEFINE_KEY(NONCONVERT, 0x1D),
        DEFINE_KEY(ACCEPT, 0x1E),
        DEFINE_KEY(MODECHANGE, 0x1F),
        DEFINE_KEY(SPACE, 0x20),
        DEFINE_KEY(PRIOR, 0x21),
        DEFINE_KEY(NEXT, 0x22),
        DEFINE_KEY(END, 0x23),
        DEFINE_KEY(HOME, 0x24),
        DEFINE_KEY(LEFT, 0x25),
        DEFINE_KEY(UP, 0x26),
        DEFINE_KEY(RIGHT, 0x27),
        DEFINE_KEY(DOWN, 0x28),
        DEFINE_KEY(SELECT, 0x29),
        DEFINE_KEY(PRINT, 0x2A),
        DEFINE_KEY(EXECUTE, 0x2B),
        DEFINE_KEY(SNAPSHOT, 0x2C),
        DEFINE_KEY(INSERT, 0x2D),
        DEFINE_KEY(DELETE, 0x2E),
        DEFINE_KEY(HELP, 0x2F),
        DEFINE_KEY(A, 0x41),
        DEFINE_KEY(B, 0x42),
        DEFINE_KEY(C, 0x43),
        DEFINE_KEY(D, 0x44),
        DEFINE_KEY(E, 0x45),
        DEFINE_KEY(F, 0x46),
        DEFINE_KEY(G, 0x47),
        DEFINE_KEY(H, 0x48),
        DEFINE_KEY(I, 0x49),
        DEFINE_KEY(J, 0x4A),
        DEFINE_KEY(K, 0x4B),
        DEFINE_KEY(L, 0x4C),
        DEFINE_KEY(M, 0x4D),
        DEFINE_KEY(N, 0x4E),
        DEFINE_KEY(O, 0x4F),
        DEFINE_KEY(P, 0x50),
        DEFINE_KEY(Q, 0x51),
        DEFINE_KEY(R, 0x52),
        DEFINE_KEY(S, 0x53),
        DEFINE_KEY(T, 0x54),
        DEFINE_KEY(U, 0x55),
        DEFINE_KEY(V, 0x56),
        DEFINE_KEY(W, 0x57),
        DEFINE_KEY(X, 0x58),
        DEFINE_KEY(Y, 0x59),
        DEFINE_KEY(Z, 0x5A),
        DEFINE_KEY(LWIN, 0x5B),
        DEFINE_KEY(RWIN, 0x5C),
        DEFINE_KEY(APPS, 0x5D),
        DEFINE_KEY(SLEEP, 0x5F),
        DEFINE_KEY(NUMPAD0, 0x60),
        DEFINE_KEY(NUMPAD1, 0x61),
        DEFINE_KEY(NUMPAD2, 0x62),
        DEFINE_KEY(NUMPAD3, 0x63),
        DEFINE_KEY(NUMPAD4, 0x64),
        DEFINE_KEY(NUMPAD5, 0x65),
        DEFINE_KEY(NUMPAD6, 0x66),
        DEFINE_KEY(NUMPAD7, 0x67),
        DEFINE_KEY(NUMPAD8, 0x68),
        DEFINE_KEY(NUMPAD9, 0x69),
        DEFINE_KEY(MULTIPLY, 0x6A),
        DEFINE_KEY(ADD, 0x6B),
        DEFINE_KEY(SEPARATOR, 0x6C),
        DEFINE_KEY(SUBTRACT, 0x6D),
        DEFINE_KEY(DECIMAL, 0x6E),
        DEFINE_KEY(DIVIDE, 0x6F),
        DEFINE_KEY(F1, 0x70),
        DEFINE_KEY(F2, 0x71),
        DEFINE_KEY(F3, 0x72),
        DEFINE_KEY(F4, 0x73),
        DEFINE_KEY(F5, 0x74),
        DEFINE_KEY(F6, 0x75),
        DEFINE_KEY(F7, 0x76),
        DEFINE_KEY(F8, 0x77),
        DEFINE_KEY(F9, 0x78),
        DEFINE_KEY(F10, 0x79),
        DEFINE_KEY(F11, 0x7A),
        DEFINE_KEY(F12, 0x7B),
        DEFINE_KEY(F13, 0x7C),
        DEFINE_KEY(F14, 0x7D),
        DEFINE_KEY(F15, 0x7E),
        DEFINE_KEY(F16, 0x7F),
        DEFINE_KEY(F17, 0x80),
        DEFINE_KEY(F18, 0x81),
        DEFINE_KEY(F19, 0x82),
        DEFINE_KEY(F20, 0x83),
        DEFINE_KEY(F21, 0x84),
        DEFINE_KEY(F22, 0x85),
        DEFINE_KEY(F23, 0x86),
        DEFINE_KEY(F24, 0x87),
        DEFINE_KEY(NUMLOCK, 0x90),
        DEFINE_KEY(SCROLL, 0x91),
        DEFINE_KEY(NUMPAD_EQUAL, 0x92),
        DEFINE_KEY(LSHIFT, 0xA0),
        DEFINE_KEY(RSHIFT, 0xA1),
        DEFINE_KEY(LCONTROL, 0xA2),
        DEFINE_KEY(RCONTROL, 0xA3),
        DEFINE_KEY(LMENU, 0xA4),
        DEFINE_KEY(RMENU, 0xA5),
        DEFINE_KEY(SEMICOLON, 0xBA),
        DEFINE_KEY(PLUS, 0xBB),
        DEFINE_KEY(COMMA, 0xBC),
        DEFINE_KEY(MINUS, 0xBD),
        DEFINE_KEY(PERIOD, 0xBE),
        DEFINE_KEY(SLASH, 0xBF),
        DEFINE_KEY(GRAVE, 0xC0),
        MAX_KEYS
    };

    // Mouse buttons and mouse position
    struct MouseState 
    {
        int16_t x;
        int16_t y;
        uint8_t buttons[static_cast<size_t>(MouseButton::MaxButtons)];
    };

    struct InputSystemState 
    {
        // Each key has a pair of callbacks for press and release
        BlitCL::StaticArray<BlitCL::Pfn<void>, 256> keyPressCallbacks;

        BlitCL::StaticArray<BlitCL::Pfn<void>, 256> keyReleaseCallbacks;

        uint8_t currentKeyboard[256];// Currently pressed boolean for each key 
        uint8_t previousKeyboard[256];// Previously pressed boolean for each key

        // Mouse state
        MouseState currentMouse;
        MouseState previousMouse;

        BlitCL::StaticArray<BlitCL::Pfn<void, int16_t, int16_t>, 3> mouseReleaseCallbacks;
        BlitCL::StaticArray<BlitCL::Pfn<void, int16_t, int16_t>, 3> mousePressCallbacks;

        InputSystemState();

        ~InputSystemState();

        inline static InputSystemState* GetState() { return s_pInputSystemState; }

        static InputSystemState* s_pInputSystemState;
    };

	// Initializes the press function pointer for the given key
    inline void RegisterKeyPressCallback(BlitKey key, BlitCL::Pfn<void> callback) 
    {
        InputSystemState::GetState()->keyPressCallbacks[size_t(key)] = callback;
    }
    
    // Initializes the release function pointer for the given key
    inline void RegisterKeyReleaseCallback(BlitKey key, BlitCL::Pfn<void> callback)
    {
        InputSystemState::GetState()->keyReleaseCallbacks[size_t(key)] = callback;
    }

    // Initializes both the release and press function pointers for the given key
    inline void RegisterKeyPressAndReleaseCallback(BlitKey key,
        BlitCL::Pfn<void> press, BlitCL::Pfn<void> release
    )
    {
        InputSystemState::GetState()->keyPressCallbacks[size_t(key)] = press;
        InputSystemState::GetState()->keyReleaseCallbacks[size_t(key)] = release;
    }

	// Accesses the press function pointer in the KEYth element 
    inline void CallKeyPressFunction(BlitKey key)
    {
        auto func = InputSystemState::GetState()->keyPressCallbacks[size_t(key)];
        if(func.IsFunctional())
            func();
    }

    // Accesses the release function pointer in the KEYth element
    inline void CallKeyReleaseFunction(BlitKey key)
    {
        auto func = InputSystemState::GetState()->keyReleaseCallbacks[size_t(key)];
        if(func.IsFunctional())
            func();
    }

    // Called when an input event for a specific key is encountered
    inline void InputProcessKey(BlitKey key, uint8_t bPressed)
    {
        InputSystemState* pState = InputSystemState::GetState();

        // Checks If the key has not already been given to the function in this frame with the same state
        if (pState->currentKeyboard[static_cast<size_t>(key)] != bPressed)
        {
			// Changes the key state to bPressed and calls the function registered to the key
            pState->currentKeyboard[static_cast<size_t>(key)] = bPressed;
            if (bPressed)
                CallKeyPressFunction(key);
            else
                CallKeyReleaseFunction(key);
        }
    }

    // Initializes the press function pointer for the given key
    inline void RegisterMouseButtonPressCallback(MouseButton button, BlitCL::Pfn<void, int16_t, int16_t> callback)
    {
		InputSystemState::GetState()->mousePressCallbacks[uint8_t(button)] = callback;
    }

    // Initializes the release function pointer for the given key
    inline void RegisterMouseButtonReleaseCallback(MouseButton button, BlitCL::Pfn<void, int16_t, int16_t> callback)
    {
        InputSystemState::GetState()->mouseReleaseCallbacks[uint8_t(button)] = callback;
    }

    // Initializes both the release and press function pointers for the given key
    inline void RegisterMouseButtonPressAndReleaseCallback(MouseButton button,
        BlitCL::Pfn<void, int16_t, int16_t> press, BlitCL::Pfn<void, int16_t, int16_t> release
    )
    {
        InputSystemState::GetState()->mousePressCallbacks[uint8_t(button)] = press;
        InputSystemState::GetState()->mouseReleaseCallbacks[uint8_t(button)] = release;
    }

    // Accesses the press function pointer in the KEYth element 
    inline void CallMouseButtonPressFunction(MouseButton button, int16_t mouseX, int16_t mouseY)
    {
        auto func = InputSystemState::GetState()->mousePressCallbacks[uint8_t(button)];
        if (func.IsFunctional())
            func(mouseX, mouseY);
    }

    // Accesses the release function pointer in the KEYth element
    inline void CallMouseButtonReleaseFunction(MouseButton button, int16_t mouseX, int16_t mouseY)
    {
        auto func = InputSystemState::GetState()->mouseReleaseCallbacks[uint8_t(button)];
        if (func.IsFunctional())
            func(mouseX, mouseY);
    }

    inline void InputProcessButton(MouseButton button, uint8_t bPressed)
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

    void UpdateInput(double deltaTime);

    // Keyboard input
    uint8_t GetCurrentKeyState(BlitKey key);
    uint8_t GetPreviousKeyState(BlitKey key);

    // mouse input
    uint8_t GetCurrentMouseButtonState(MouseButton button);
    uint8_t GetPreviousMouseButtonState(MouseButton button);

    void GetMousePosition(int32_t* x, int32_t* y);
    void GetPreviousMousePosition(int32_t* x, int32_t* y);

    void InputProcessMouseMove(int16_t x, int16_t y);

    void InputProcessMouseWheel(int8_t zDelta);
}