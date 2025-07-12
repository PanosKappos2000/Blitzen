#include "blitController.h"

namespace BlitzenCore
{
	void Controller::InitControllerPFNs()
	{
        for (uint32_t pfn = 0; pfn < Ce_KeyCallbackCount; ++pfn)
        {
            m_keyData[pfn].m_PFNHeld = [](BlitzenEngine::Resident, float)->BlitEventType { return BlitEventType::MaxTypes; };
            m_keyData[pfn].m_PFNTap = [](BlitzenEngine::Resident, float)->BlitEventType { return BlitEventType::MaxTypes; };
        }

        for (uint32_t pfn = 0; pfn < Ce_MouseButtonPFNCount; ++pfn)
        {
            m_mousePressPFNs[pfn] = [](BlitzenEngine::Resident, float, int16_t, int16_t)->BlitEventType { return BlitEventType::MaxTypes; };
            m_mouseReleasePFNs[pfn] = [](BlitzenEngine::Resident, float, int16_t, int16_t)->BlitEventType { return BlitEventType::MaxTypes; };
        }

        m_mouseMovePFNs = [](BlitzenEngine::Resident, float, int32_t, int32_t)->BlitEventType { return BlitEventType::MaxTypes; };

        m_mouseWheelPFNs = [](BlitzenEngine::Resident, float, int8_t)->BlitEventType { return BlitEventType::MaxTypes; };
	}
    
    void Controller::DispatchHeldDownKeyEvents(float deltaTime)
    {
        for (uint32_t hldID = 0; hldID < m_registeredKeyHeldCount; ++hldID)
        {
            auto& keyData = m_keyData[m_keyHeldIdxs[hldID]];
            if (keyData.m_heldDownFlags != BLIT_FAT_FALSE)
            {
                keyData.m_PFNHeld(m_resident, deltaTime);
            }
        }
    }
}