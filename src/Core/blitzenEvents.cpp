#include "blitInput.h"
#include "Engine/blitzenEngine.h"

namespace BlitzenCore
{
    EventSystemState::EventSystemState(void** PP)
    {
        ppContext = PP;

        for (uint32_t i = 0; i < uint8_t(BlitEventType::MaxTypes); ++i)
        {
            m_eventCallbacks[i] = [](void**, BlitEventType type)->uint8_t {return false; };
        }
    }

    void RegisterEvent(EventSystemState* pContext, BlitEventType type, EventCallback eventCallback)
    {
        if (type == BlitEventType::KeyPressed || type == BlitEventType::KeyReleased || type == BlitEventType::MouseButtonPressed || type == BlitEventType::MouseButtonReleased
            || type == BlitEventType::MouseMoved || type == BlitEventType::MouseWheel)
        {
            return;
        }

        pContext->m_eventCallbacks[size_t(type)] = eventCallback;
    }

    InputSystemState::InputSystemState(void** pp)
    {
        ppContext = pp;

        for (uint32_t i = 0; i < Ce_KeyCallbackCount; ++i)
        {
            keyPressCallbacks[i] = [](void**) {BLIT_INFO("No callback assigned"); };
            keyReleaseCallbacks[i] = [](void**) {};
        }

        for (uint32_t i = 0; i < uint8_t(MouseButton::MaxButtons); i++)
        {
            mousePressCallbacks[i] = [](void**, int16_t, int16_t) {};
            mouseReleaseCallbacks[i] = [](void**, int16_t, int16_t) {};
        }

        mouseMoveCallback = [](void**, int16_t, int16_t, int16_t, int16_t) {};

        mouseWheelCallback = [](void**, int8_t) {};
    }

    void InputSystemState::InputProcessKey(BlitKey key, bool bPressed)
    {
        auto idx = static_cast<uint16_t>(key);
        // If the state changed, fire callback
        if (m_currentKeyboard[idx] != bPressed)
        {
            m_currentKeyboard[idx] = bPressed;
            if (bPressed)
            {
                keyPressCallbacks[idx](ppContext);
            }
            else
            {
                keyReleaseCallbacks[idx](ppContext);
            }
        }
    }

    void InputSystemState::InputProcessMouseMove(int16_t x, int16_t y)
    {
        if (m_currentMouse.x != x || m_currentMouse.y != y)
        {
            m_previousMouse.x = m_currentMouse.x;
            m_previousMouse.y = m_currentMouse.y;
            m_currentMouse.x = x;
            m_currentMouse.y = y;

            mouseMoveCallback(ppContext, m_currentMouse.x, m_currentMouse.y, m_previousMouse.x, m_previousMouse.y);
        }
    }

    void InputSystemState::InputProcessButton(MouseButton button, bool bPressed)
    {
        auto idx = static_cast<uint8_t>(button);
        // If the state changed, fire callback.
        if (m_currentMouse.buttons[idx] != bPressed)
        {
            m_currentMouse.buttons[idx] = bPressed;
            if (bPressed)
            {
                mousePressCallbacks[idx](ppContext, m_currentMouse.x, m_currentMouse.y);
            }
            else
            {
                mouseReleaseCallbacks[idx](ppContext, m_currentMouse.x, m_currentMouse.y);
            }
        }
    }

    void InputSystemState::InputProcessMouseWheel(int8_t zDelta)
    {
        mouseWheelCallback(ppContext, zDelta);
    }

    void RegisterKeyPressCallback(InputSystemState* pContext, BlitKey key, KeyPressCallback callback)
    {
        pContext->keyPressCallbacks[size_t(key)] = callback;
    }

    void RegisterKeyReleaseCallback(InputSystemState* pContext, BlitKey key, KeyReleaseCallback callback)
    {
        pContext->keyReleaseCallbacks[size_t(key)] = callback;
    }

    void RegisterKeyPressAndReleaseCallback(InputSystemState* pContext, BlitKey key, KeyPressCallback press, KeyReleaseCallback release)
    {
        pContext->keyPressCallbacks[size_t(key)] = press;
        pContext->keyReleaseCallbacks[size_t(key)] = release;
    }

    void RegisterMouseButtonPressCallback(InputSystemState* pContext, MouseButton button, MouseButtonPressCallback callback)
    {
        pContext->mousePressCallbacks[uint8_t(button)] = callback;
    }

    void RegisterMouseButtonReleaseCallback(InputSystemState* pContext, MouseButton button, MouseButtonReleaseCallback callback)
    {
        pContext->mouseReleaseCallbacks[uint8_t(button)] = callback;
    }

    void RegisterMouseButtonPressAndReleaseCallback(InputSystemState* pContext, MouseButton button, MouseButtonPressCallback press, MouseButtonReleaseCallback release)
    {
        pContext->mousePressCallbacks[uint8_t(button)] = press;
        pContext->mouseReleaseCallbacks[uint8_t(button)] = release;
    }

    void RegisterMouseWheelCallback(InputSystemState* pContext, MouseWheelCallbackType callback)
    {
        pContext->mouseWheelCallback = callback;
    }

    void RegisterMouseMoveCallback(InputSystemState* pContext, MouseMoveCallbackType callback)
    {
        pContext->mouseMoveCallback = callback;
    }
}