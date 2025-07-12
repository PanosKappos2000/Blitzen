#include "blitFrameEvents.h"

namespace BlitzenCore
{
    void FrameEventManager::RegisterFrameEvent(BlitzenEngine::Resident resident, FrameEventPfn function)
    {
        auto& frameEvent = m_frameEvents[m_frameEventCount++];
        frameEvent.m_function = function;
        frameEvent.m_resident = resident;
    }
}