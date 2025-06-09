#pragma once

#include "Core/blitzenEngine.h"
#include "BlitCL/blitPfn.h"
#include "blitEventType.h"
#include "Core/BlitzenWorld/blitzenWorld.h"

namespace BlitzenCore
{
    using KeyPressCallback = BlitCL::Pfn<BlitEventType, BlitzenWorld::BlitzenWorldContext&>;
    using KeyReleaseCallback = BlitCL::Pfn<BlitEventType, BlitzenWorld::BlitzenWorldContext&>;

    using MouseButtonPressCallback = BlitCL::Pfn<BlitEventType, BlitzenWorld::BlitzenWorldContext&, int16_t, int16_t>;
    using MouseButtonReleaseCallback = BlitCL::Pfn<BlitEventType, BlitzenWorld::BlitzenWorldContext&, int16_t, int16_t>;

    using MouseMoveCallbackType = BlitCL::Pfn<BlitEventType, BlitzenWorld::BlitzenWorldContext&, int16_t, int16_t, int16_t, int16_t>;
    using MouseWheelCallbackType = BlitCL::Pfn<BlitEventType, BlitzenWorld::BlitzenWorldContext&, int8_t>;

	struct Controller
	{
        KeyPressCallback m_keyPressPFNs[Ce_KeyCallbackCount];
        KeyReleaseCallback m_keyReleasePFNs[Ce_KeyCallbackCount];

        MouseMoveCallbackType m_mouseMovePFNs;

        MouseWheelCallbackType m_mouseWheelPFNs;

        MouseButtonReleaseCallback m_mousePressPFNs[Ce_MouseButtonPFNCount];
        MouseButtonPressCallback m_mouseReleasePFNs[Ce_MouseButtonPFNCount];
	};

    inline void InitControllerPFNs(Controller& controller)
    {
        for (uint32_t pfn = 0; pfn < Ce_KeyCallbackCount; ++pfn)
        {
            controller.m_keyPressPFNs[pfn] = [](BlitzenWorld::BlitzenWorldContext&)->BlitEventType { return BlitEventType::MaxTypes; };
            controller.m_keyReleasePFNs[pfn] = [](BlitzenWorld::BlitzenWorldContext&)->BlitEventType { return BlitEventType::MaxTypes; };
        }

        for (uint32_t pfn = 0; pfn < Ce_MouseButtonPFNCount; ++pfn)
        {
            controller.m_mousePressPFNs[pfn] = [](BlitzenWorld::BlitzenWorldContext&, int16_t, int16_t)->BlitEventType { return BlitEventType::MaxTypes; };
            controller.m_mouseReleasePFNs[pfn] = [](BlitzenWorld::BlitzenWorldContext&, int16_t, int16_t)->BlitEventType { return BlitEventType::MaxTypes; };
        }

        controller.m_mouseMovePFNs = [](BlitzenWorld::BlitzenWorldContext&, int16_t, int16_t, int16_t, int16_t)->BlitEventType { return BlitEventType::MaxTypes; };

        controller.m_mouseWheelPFNs = [](BlitzenWorld::BlitzenWorldContext&, int8_t)->BlitEventType { return BlitEventType::MaxTypes; };
    }
}