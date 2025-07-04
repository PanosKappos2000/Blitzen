#include "blitController.h"

namespace BlitzenCore
{
	void Controller::InitControllerPFNs()
	{
        for (uint32_t pfn = 0; pfn < Ce_KeyCallbackCount; ++pfn)
        {
            m_keyPressPFNs[pfn] = [](BlitzenEngine::Resident, float)->BlitEventType { return BlitEventType::MaxTypes; };
            m_keyReleasePFNs[pfn] = [](BlitzenEngine::Resident, float)->BlitEventType { return BlitEventType::MaxTypes; };
        }

        for (uint32_t pfn = 0; pfn < Ce_MouseButtonPFNCount; ++pfn)
        {
            m_mousePressPFNs[pfn] = [](BlitzenEngine::Resident, float, int16_t, int16_t)->BlitEventType { return BlitEventType::MaxTypes; };
            m_mouseReleasePFNs[pfn] = [](BlitzenEngine::Resident, float, int16_t, int16_t)->BlitEventType { return BlitEventType::MaxTypes; };
        }

        m_mouseMovePFNs = [](BlitzenEngine::Resident, float, int16_t, int16_t)->BlitEventType { return BlitEventType::MaxTypes; };

        m_mouseWheelPFNs = [](BlitzenEngine::Resident, float, int8_t)->BlitEventType { return BlitEventType::MaxTypes; };
	}
    
}