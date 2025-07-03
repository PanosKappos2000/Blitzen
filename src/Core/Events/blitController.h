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

        BlitzenEngine::Camera m_engineCamera;

        void InitControllerPFNs();
	};

    BlitEventType BLITZEN_ENGINE_CONTROLLED_RESIDENT_VIEW(BlitzenEngine::Resident resident, int16_t screenCoordX, int16_t screenCoordY, BlitKey key);
}