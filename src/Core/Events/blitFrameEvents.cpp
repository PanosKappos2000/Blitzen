#include "blitFrameEvents.h"

namespace BlitzenCore
{
    void FrameEventManager::RegisterFrameEvent(BlitzenEngine::WORLD_VARIABLE worldVariable, FrameEventPfn function)
    {
        auto& frameEvent = m_frameEvents[m_frameEventCount++];
        frameEvent.m_function = function;
        frameEvent.m_worldVariableArg = worldVariable;
    }
}