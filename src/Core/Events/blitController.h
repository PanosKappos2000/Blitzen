#pragma once
#include "BlitCL/blitPfn.h"
#include "blitEventType.h"
#include "blitKeys.h"
#include "Renderer/WORLD/blitzenWorld.h"

namespace BlitzenCore
{
    using KeyPressCallback = BlitCL::Pfn<BlitEventType, BlitzenWorld::WORLD_blit*>;
    using KeyReleaseCallback = BlitCL::Pfn<BlitEventType, BlitzenWorld::WORLD_blit*>;

    using MouseButtonPressCallback = BlitCL::Pfn<BlitEventType, BlitzenWorld::WORLD_blit*, int16_t, int16_t>;
    using MouseButtonReleaseCallback = BlitCL::Pfn<BlitEventType, BlitzenWorld::WORLD_blit*, int16_t, int16_t>;

    using MouseMoveCallbackType = BlitCL::Pfn<BlitEventType, BlitzenWorld::WORLD_blit*, int16_t, int16_t>;

    using MouseWheelCallbackType = BlitCL::Pfn<BlitEventType, BlitzenWorld::WORLD_blit*, int8_t>;

	class Controller
	{
    public:

        KeyPressCallback m_keyPressPFNs[Ce_KeyCallbackCount];
        KeyReleaseCallback m_keyReleasePFNs[Ce_KeyCallbackCount];

        MouseMoveCallbackType m_mouseMovePFNs;

        MouseWheelCallbackType m_mouseWheelPFNs;

        MouseButtonReleaseCallback m_mousePressPFNs[Ce_MouseButtonPFNCount];
        MouseButtonPressCallback m_mouseReleasePFNs[Ce_MouseButtonPFNCount];

        void InitControllerPFNs();
	};

    class ResidentController
    {
    public:
        BlitCL::Pfn<BlitEventType, BlitzenEngine::Resident, int16_t, int16_t> CONTROL[Ce_KeyCallbackCount];

        inline BlitEventType operator () (uint32_t id, BlitzenEngine::Resident resident, int16_t screenCoordX, int16_t screenCoordY)
        {
            return CONTROL[id](resident, screenCoordX, screenCoordY);
        }

        inline void REGISTER(uint32_t id, BlitCL::Pfn<BlitEventType, BlitzenEngine::Resident, int16_t, int16_t> FUNC)
        {
            CONTROL[id] = FUNC;
        }

    private:
        BlitzenEngine::Resident m_resident;
        BlitzenEngine::Camera m_gameCamera;
    };

    BlitEventType BLITZEN_ENGINE_CONTROLLED_RESIDENT_VIEW(BlitzenEngine::Resident resident, int16_t screenCoordX, int16_t screenCoordY, BlitKey key);
}