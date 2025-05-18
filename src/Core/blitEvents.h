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

    using EventCallback = BlitCL::Pfn<uint8_t, void**, BlitEventType>;

    class EventSystemState 
    {
    public:

        EventSystemState(void** PP);

        inline ~EventSystemState() { }

        inline bool operator () (BlitEventType type, void* pSender, const EventContext& context) const
        {
            auto event = m_eventCallbacks[size_t(type)];
            return event(ppContext, type);
        }

        inline bool FireEvent(BlitEventType type) const
        {
            auto event = m_eventCallbacks[size_t(type)];
            return event(ppContext, type);
        }

        EventCallback m_eventCallbacks[uint8_t(BlitEventType::MaxTypes)];
        
        void** ppContext;
    };

    // Adds a new RegisteredEvent to the eventState event types array
    void RegisterEvent(EventSystemState* pContext, BlitEventType type, EventCallback eventCallback);
}