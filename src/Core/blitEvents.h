#pragma once
#include "BlitCL/blitPfn.h"
#include "blitLogger.h"

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

    enum class BlitEventType : uint8_t
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

    using EventCallbackType = BlitCL::Pfn<uint8_t, BlitEventType, void*, void*, const EventContext&>;
    struct RegisteredEvent 
    {
        void* pListener;
        EventCallbackType callback{ [](BlitEventType, void*, void*, const EventContext&)->uint8_t {
            BLIT_INFO("No callback assigned")
            return false;
        }};
    };

    class EventSystemState 
    {
    public:

        RegisteredEvent eventTypes[uint8_t(BlitEventType::MaxTypes)];

        inline EventSystemState(){ s_pEventSystemState = this; }

        inline ~EventSystemState() {s_pEventSystemState = nullptr; }

        inline bool operator () (BlitEventType type, void* pSender, const EventContext& context) const
        {
            auto event = eventTypes[size_t(type)];
            return event.callback(type, pSender, event.pListener, context);
        }

        inline bool FireEvent(BlitEventType type, void* pSender, const EventContext& context) const
        {
            auto event = eventTypes[size_t(type)];
            return event.callback(type, pSender, event.pListener, context);
        }

        inline static EventSystemState* GetState() { return s_pEventSystemState; }


    private:
        inline static EventSystemState* s_pEventSystemState;
    };

    // Adds a new RegisteredEvent to the eventState event types array
    void RegisterEvent(BlitEventType type, void* pListener, EventCallbackType eventCallback);
}