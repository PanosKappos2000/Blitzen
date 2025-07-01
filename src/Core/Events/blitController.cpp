#include "blitController.h"

namespace BlitzenCore
{
	void Controller::InitControllerPFNs()
	{
        for (uint32_t pfn = 0; pfn < Ce_KeyCallbackCount; ++pfn)
        {
            m_keyPressPFNs[pfn] = [](BlitzenWorld::WORLD_blit*)->BlitEventType { return BlitEventType::MaxTypes; };
            m_keyReleasePFNs[pfn] = [](BlitzenWorld::WORLD_blit*)->BlitEventType { return BlitEventType::MaxTypes; };
        }

        for (uint32_t pfn = 0; pfn < Ce_MouseButtonPFNCount; ++pfn)
        {
            m_mousePressPFNs[pfn] = [](BlitzenWorld::WORLD_blit*, int16_t, int16_t)->BlitEventType { return BlitEventType::MaxTypes; };
            m_mouseReleasePFNs[pfn] = [](BlitzenWorld::WORLD_blit*, int16_t, int16_t)->BlitEventType { return BlitEventType::MaxTypes; };
        }

        m_mouseMovePFNs = [](BlitzenWorld::WORLD_blit*, int16_t, int16_t)->BlitEventType { return BlitEventType::MaxTypes; };

        m_mouseWheelPFNs = [](BlitzenWorld::WORLD_blit*, int8_t)->BlitEventType { return BlitEventType::MaxTypes; };
	}
}