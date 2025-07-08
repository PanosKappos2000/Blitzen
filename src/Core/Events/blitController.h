#pragma once
#include "BlitCL/blitPfn.h"
#include "blitEventType.h"
#include "blitKeys.h"
#include "Renderer/WORLD/blitzenWorld.h"

namespace BlitzenCore
{
    using KeyCallback = BlitCL::Pfn<BlitEventType, BlitzenEngine::Resident, float>;
    using MouseButtonPressCallback = BlitCL::Pfn<BlitEventType, BlitzenEngine::Resident, float, int16_t, int16_t>;
    using MouseButtonReleaseCallback = BlitCL::Pfn<BlitEventType, BlitzenEngine::Resident, float, int16_t, int16_t>;
    using MouseMoveCallbackType = BlitCL::Pfn<BlitEventType, BlitzenEngine::Resident, float, int32_t, int32_t>;
    using MouseWheelCallbackType = BlitCL::Pfn<BlitEventType, BlitzenEngine::Resident, float, int8_t>;

    enum class KeyCallbackType
    {
        PRESS,
        RELEASE,
        HOLD
    };

    struct KeyData
    {
        BlitKey m_keycode;
        KeyCallback m_PFN;
        BlitzenCore::BIG_BOOL m_heldDownFlags = BLIT_FAT_FALSE;
        KeyCallbackType m_keyCallbackType;
    };

	class Controller
	{
    public:

		KeyData m_keyData[Ce_KeyCallbackCount];
		uint32_t m_keyHeldIdxs[Ce_KeyCallbackCount]{};
        uint32_t m_registeredKeyHeldCount = 0;
        MouseMoveCallbackType m_mouseMovePFNs;
        MouseWheelCallbackType m_mouseWheelPFNs;
        MouseButtonReleaseCallback m_mousePressPFNs[Ce_MouseButtonPFNCount];
        MouseButtonPressCallback m_mouseReleasePFNs[Ce_MouseButtonPFNCount];
        BlitzenEngine::Resident m_resident;
        

        void InitControllerPFNs();

        inline BlitEventType KEYPRESS(uint32_t idx, float deltaTime)
        {
            return m_keyData[idx].m_PFN(m_resident, deltaTime);
        }
        inline BlitEventType KEYRELEASE(uint32_t idx, float deltaTime)
        {
            return m_keyData[idx].m_PFN(m_resident, deltaTime);
        }
        inline BlitEventType MOUSEMOVE(int32_t xAxisMovement, int32_t yAxisMovement, float deltaTime)
        {
            return m_mouseMovePFNs(m_resident, deltaTime, xAxisMovement, yAxisMovement);
        }
        inline BlitEventType WHEEL(int8_t zDelta, float deltaTime)
        {
            return m_mouseWheelPFNs(m_resident, deltaTime, zDelta);
        }
        inline BlitEventType MBPRESS(uint32_t idx, int16_t mouseX, int16_t mouseY, float deltaTime)
        {
            return m_mousePressPFNs[idx](m_resident, deltaTime, mouseX, mouseY);
        }
        inline BlitEventType MBRELEASE(uint32_t idx, int16_t mouseX, int16_t mouseY, float deltaTime)
        {
            return m_mouseReleasePFNs[idx](m_resident, deltaTime, mouseX, mouseY);
        }

        inline void KEYHOLD(uint32_t idx, float deltaTime)
        {
			m_keyData[idx].m_heldDownFlags = BLIT_FAT_TRUE;
		}
        inline void KEYRELEASEHOLD(uint32_t idx, float deltaTime)
        {
            m_keyData[idx].m_heldDownFlags = BLIT_FAT_FALSE;
		}

        void DispatchHeldDownKeyEvents(float deltaTime);
	};

    BlitEventType BLITZEN_ENGINE_CONTROLLED_RESIDENT_VIEW(BlitzenEngine::Resident resident, int16_t screenCoordX, int16_t screenCoordY, BlitKey key);
}