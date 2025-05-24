#include "blitPlatformContext.h"

#if defined(linux)
#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <xcb/xcb.h>
#include <X11/keysym.h>
#include <sys/time.h>
#if _POSIX_C_SOURCE >= 199309L
#include <time.h>  // nanosleep
#else
#include <unistd.h>  // usleep
#endif
#include "blitPlatform.h"
#include "Core/Events/blitEvents.h"
#include "Renderer/BlitzenVulkan/vulkanData.h"
#include <vulkan/vulkan_xcb.h>
#include "Core/blitzenEngine.h"
#include "Renderer/Interface/blitRenderer.h"


namespace BlitzenPlatform
{

        // Key translation
        BlitzenCore::BlitKey TranslateKeycode(uint32_t xKeycode);

        bool PlatformStartup(const char* appName, void* pPlatform, void* pEvents, void* pRenderer)
        {
            auto P_HANDLE{reinterpret_cast<PlatformContext*>(pPlatform)};
            P_HANDLE->m_pEvents = pEvents;

            // DISPLAY, KEY REPEATS OFF
            P_HANDLE->m_pDisplay = XOpenDisplay(nullptr);
            XAutoRepeatOff(P_HANDLE->m_pDisplay);

            // CONNECTION
            P_HANDLE->m_pConnection = XGetXCBConnection(P_HANDLE->m_pDisplay);

            if (xcb_connection_has_error(P_HANDLE->m_pConnection)) 
            {
                BLIT_FATAL("Failed to connect to X server via XCB.");
                return false;
            }

            const struct xcb_setup_t* setup = xcb_get_setup(P_HANDLE->m_pConnection);

            // SCREEN LOOP
            auto iterator = xcb_setup_roots_iterator(setup);
            int32_t screenP = 0;
            for (int32_t screen = screenP; screen > 0; screen--)
            {
                xcb_screen_next(&iterator);
            }

            // GET SCREEN
            P_HANDLE->m_pScreen = iterator.data;

            // EVENT TYPES
            // XCB_CW_BACK_PIXEL = filling then window bg with a single colour
            // XCB_CW_EVENT_MASK is required.
            uint32_t eventMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
            uint32_t eventValues = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
            XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

            // Values to be sent over XCB (bg colour, events)
            uint32_t valueList[] = { P_HANDLE->m_pScreen->black_pixel, eventValues };

            // WINDOW
            P_HANDLE->m_window = xcb_generate_id(P_HANDLE->m_pConnection);
            auto cookie = xcb_create_window(P_HANDLE->m_pConnection,XCB_COPY_FROM_PARENT, P_HANDLE->m_window, P_HANDLE->m_pScreen->root, 
            BlitzenCore::Ce_WindowStartingX, BlitzenCore::Ce_WindowStartingY, BlitzenCore::Ce_InitialWindowWidth, BlitzenCore::Ce_InitialWindowHeight, 0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT, P_HANDLE->m_pScreen->root_visual, eventMask, valueList);

            // TITLE
            xcb_change_property(P_HANDLE->m_pConnection, XCB_PROP_MODE_REPLACE, P_HANDLE->m_wmProtocols, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(appName), appName);

            // Tell the server to notify when the window manager attempts to destroy the window.
            auto wm_delete_cookie = xcb_intern_atom(P_HANDLE->m_pConnection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
            auto wm_protocols_cookie = xcb_intern_atom(P_HANDLE->m_pConnection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
            auto wm_delete_reply = xcb_intern_atom_reply(P_HANDLE->m_pConnection, wm_delete_cookie, nullptr);
            auto wm_protocols_reply = xcb_intern_atom_reply(P_HANDLE->m_pConnection, wm_protocols_cookie, nullptr);
            P_HANDLE->m_wmDeleteWin = wm_delete_reply->atom;
            P_HANDLE->m_wmProtocols = wm_protocols_reply->atom;

            // i don't remember
            xcb_change_property(P_HANDLE->m_pConnection, XCB_PROP_MODE_REPLACE, P_HANDLE->m_window, wm_protocols_reply->atom, 4, 32, 1, &wm_delete_reply->atom);

            // MAP WINDOW TO SCREEN
            xcb_map_window(P_HANDLE->m_pConnection, P_HANDLE->m_window);

            // Flushes the stream
            int32_t streamResult = xcb_flush(P_HANDLE->m_pConnection);
            if (streamResult <= 0)
            {
                BLIT_FATAL("An error occurred when flusing the stream: %d", streamResult);
                return false;
            }

            auto pBackendRenderer = reinterpret_cast<BlitzenEngine::RendererPtrType>(pRenderer);
            if (!pBackendRenderer->Init(BlitzenCore::Ce_InitialWindowWidth, BlitzenCore::Ce_InitialWindowHeight, P_HANDLE))
            {
                BLIT_FATAL("Failed to initialize rendering API");
                return false;
            }

            // success
            return true;
        }

        void BlitzenSleep(uint64_t ms)
        {
            #if _POSIX_C_SOURCE >= 199309L
            struct timespec ts;
            ts.tv_sec = ms / 1000;
            ts.tv_nsec = (ms % 1000) * 1000 * 1000;
            nanosleep(&ts, 0);
            #else
            if (ms >= 1000)
            {
                sleep(ms / 1000);
            }
            usleep((ms % 1000) * 1000);
            #endif
        }

        void PlatfrormSetupClock(BlitzenCore::WorldTimerManager* pClock)
        {
            
        }

        /*
			TIME MANAGER
        */
        double PlatformGetAbsoluteTime(double frequence) 
        {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);

            // I don't know if this is any good I did not write it
            return now.tv_sec + now.tv_nsec * 0.000000001;
        }

        /*
            LOGGING
        */
        void PlatformConsoleWrite(const char* message, uint8_t color)
        {
            // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
            const char* colorStrings[] = { "0;41", "1;31", "1;33", "1;32", "1;34", "1;30" };
            printf("\033[%sm%s\033[0m", colorStrings[color], message);
        }
        void PlatformConsoleError(const char* message, uint8_t color)
        {
            // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
            const char* colorStrings[] = { "0;41", "1;31", "1;33", "1;32", "1;34", "1;30" };
            printf("\033[%sm%s\033[0m", colorStrings[color], message);
        }

        /*
            VULKAN
        */
        uint8_t CreateVulkanSurface(VkInstance& instance, VkSurfaceKHR& surface, VkAllocationCallbacks* pAllocator, void* pPlatform)
        {
            auto P_HANDLE{ reinterpret_cast<PlatformContext*>(pPlatform) };

            VkXcbSurfaceCreateInfoKHR info{};
            info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
            info.connection = P_HANDLE->m_pConnection;
            info.window = P_HANDLE->m_window;

            VkResult res = vkCreateXcbSurfaceKHR(instance, &info, pAllocator, &surface);
            if (res != VK_SUCCESS)
                return 0;
            return 1;
        }

        /*
            EVENT SYSTEM
        */
        struct scoped_linux_event
        {
            xcb_generic_event_t* m_pEvent;

            inline ~scoped_linux_event()
            {
                if(m_pEvent)
                {
                    free(m_pEvent);
                }
            }
        };
        bool DispatchEvents(void* pPlatform) 
        {
            auto P_HANDLE{reinterpret_cast<PlatformContext*>(pPlatform)};
            auto pEventSystem{reinterpret_cast<BlitzenCore::EventSystem*>(P_HANDLE->m_pEvents)};

            xcb_client_message_event_t* pClientMessage;

            bool quitFlagged = false;

            // POLL 
            while (true) 
            {
                scoped_linux_event scopedLinuxEvent;
                scopedLinuxEvent.m_pEvent = xcb_poll_for_event(P_HANDLE->m_pConnection);

                if (!scopedLinuxEvent.m_pEvent) 
                {
                    break;
                }

                // Input events
                switch (scopedLinuxEvent.m_pEvent->response_type & ~0x80) 
                {
                case XCB_KEY_PRESS:
                case XCB_KEY_RELEASE: 
                {
                    // TRANSLATE KEY
                    auto pKbEvent = reinterpret_cast<xcb_key_press_event_t*>(scopedLinuxEvent.m_pEvent);
                    auto code = pKbEvent->detail;
                    KeySym keySym = XkbKeycodeToKeysym(P_HANDLE->m_pDisplay, (KeyCode)code, 0, code & ShiftMask ? 1 : 0);
                    auto key = TranslateKeycode(keySym);

                    // EVENT SYSTEM
                    bool pressed = scopedLinuxEvent.m_pEvent->response_type == XCB_KEY_PRESS;
                    pEventSystem->InputProcessKey(key, pressed);
                }
                // NOTE: This is not a mistake, the loop needs to break here, otherwise I will keep getting the same message
                break;

                case XCB_BUTTON_PRESS:
                case XCB_BUTTON_RELEASE: 
                {
                    auto pMouseEvent = reinterpret_cast<xcb_button_press_event_t*>(scopedLinuxEvent.m_pEvent);
                    auto mouseButton = BlitzenCore::MouseButton::MaxButtons;

                    // GET MOUSE BUTTON
                    switch (pMouseEvent->detail) 
                    {
                    case XCB_BUTTON_INDEX_1:
                        mouseButton = BlitzenCore::MouseButton::Left;
                        break;
                    case XCB_BUTTON_INDEX_2:
                        mouseButton = BlitzenCore::MouseButton::Middle;
                        break;
                    case XCB_BUTTON_INDEX_3:
                        mouseButton = BlitzenCore::MouseButton::Right;
                        break;
                    }

                    // EVENT SYSTEM
                    bool bPressed = scopedLinuxEvent.m_pEvent->response_type == XCB_BUTTON_PRESS;
                    if (mouseButton != BlitzenCore::MouseButton::MaxButtons) 
                    {
                        pEventSystem->InputProcessButton(mouseButton, bPressed);
                    }
                }
                // NOTE: This is not a mistake, the loop needs to break here, otherwise I will keep getting the same message
                break;

                case XCB_MOTION_NOTIFY: 
                {
                    auto pMouseMoveEvent = reinterpret_cast<xcb_motion_notify_event_t*>(scopedLinuxEvent.m_pEvent);
                    pEventSystem->InputProcessMouseMove(pMouseMoveEvent->event_x, pMouseMoveEvent->event_y);
                }
                // NOTE: This is not a mistake, the loop needs to break here, otherwise I will keep getting the same message
                break;

                case XCB_CONFIGURE_NOTIFY: 
                {
                    auto pConfigureEvent = reinterpret_cast<xcb_configure_notify_event_t*>(scopedLinuxEvent.m_pEvent);

                    auto& camera{ pEventSystem->m_blitzenContext.pCameraContainer->GetMainCamera()};

                    auto oldWidth = camera.transformData.windowWidth;
                    auto oldHeight = camera.transformData.windowHeight;

                    // The camera will carry the context for the window resize
                    camera.transformData.windowWidth = float(pConfigureEvent->width);
                    camera.transformData.windowHeight = float(pConfigureEvent->height);

                    if (!pEventSystem->FireEvent(BlitzenCore::BlitEventType::WindowUpdate))
                    {
                        camera.transformData.windowWidth = oldWidth;
                        camera.transformData.windowHeight = oldHeight;
                    }

                    break;
                }

                case XCB_CLIENT_MESSAGE: 
                {
                    pClientMessage = reinterpret_cast<xcb_client_message_event_t*>(scopedLinuxEvent.m_pEvent);

                    // Window close
                    if (pClientMessage->data.data32[0] == P_HANDLE->m_wmDeleteWin) 
                    {
                        quitFlagged = true;
                    }
                    break;
                }
                default:
                {
                    break;
                }
                }
            }

            return !quitFlagged;
        }

        BlitzenCore::BlitKey TranslateKeycode(uint32_t x_keycode)
        {
            switch (x_keycode)
            {
                case XK_BackSpace:
                    return BlitzenCore::BlitKey::__BACKSPACE;
                case XK_Return:
                    return BlitzenCore::BlitKey::__ENTER;
                case XK_Tab:
                    return BlitzenCore::BlitKey::__TAB;
                    //case XK_Shift: return KEY_SHIFT;
                    //case XK_Control: return KEY_CONTROL;

                case XK_Pause:
                    return BlitzenCore::BlitKey::__PAUSE;
                case XK_Caps_Lock:
                    return BlitzenCore::BlitKey::__CAPITAL;

                case XK_Escape:
                    return BlitzenCore::BlitKey::__ESCAPE;

                    // Not supported
                    // case : return KEY_CONVERT;
                    // case : return KEY_NONCONVERT;
                    // case : return KEY_ACCEPT;

                case XK_Mode_switch:
                    return BlitzenCore::BlitKey::__MODECHANGE;

                case XK_space:
                    return BlitzenCore::BlitKey::__SPACE;
                case XK_Prior:
                    return BlitzenCore::BlitKey::__PRIOR;
                case XK_Next:
                    return BlitzenCore::BlitKey::__NEXT;
                case XK_End:
                    return BlitzenCore::BlitKey::__END;
                case XK_Home:
                    return BlitzenCore::BlitKey::__HOME;
                case XK_Left:
                    return BlitzenCore::BlitKey::__LEFT;
                case XK_Up:
                    return BlitzenCore::BlitKey::__UP;
                case XK_Right:
                    return BlitzenCore::BlitKey::__RIGHT;
                case XK_Down:
                    return BlitzenCore::BlitKey::__DOWN;
                case XK_Select:
                    return BlitzenCore::BlitKey::__SELECT;
                case XK_Print:
                    return BlitzenCore::BlitKey::__PRINT;
                case XK_Execute:
                    return BlitzenCore::BlitKey::__EXECUTE;
                    // case XK_snapshot: return KEY_SNAPSHOT; // not supported
                case XK_Insert:
                    return BlitzenCore::BlitKey::__INSERT;
                case XK_Delete:
                    return BlitzenCore::BlitKey::__DELETE;
                case XK_Help:
                    return BlitzenCore::BlitKey::__HELP;

                case XK_Meta_L:
                    return BlitzenCore::BlitKey::__LWIN;  // TODO: not sure this is right
                case XK_Meta_R:
                    return BlitzenCore::BlitKey::__RWIN;
                    // case XK_apps: return KEY_APPS; // not supported

                    // case XK_sleep: return KEY_SLEEP; //not supported

                case XK_KP_0:
                    return BlitzenCore::BlitKey::__NUMPAD0;
                case XK_KP_1:
                    return BlitzenCore::BlitKey::__NUMPAD1;
                case XK_KP_2:
                    return BlitzenCore::BlitKey::__NUMPAD2;
                case XK_KP_3:
                    return BlitzenCore::BlitKey::__NUMPAD3;
                case XK_KP_4:
                    return BlitzenCore::BlitKey::__NUMPAD4;
                case XK_KP_5:
                    return BlitzenCore::BlitKey::__NUMPAD5;
                case XK_KP_6:
                    return BlitzenCore::BlitKey::__NUMPAD6;
                case XK_KP_7:
                    return BlitzenCore::BlitKey::__NUMPAD7;
                case XK_KP_8:
                    return BlitzenCore::BlitKey::__NUMPAD8;
                case XK_KP_9:
                    return BlitzenCore::BlitKey::__NUMPAD9;
                case XK_multiply:
                    return BlitzenCore::BlitKey::__MULTIPLY;
                case XK_KP_Add:
                    return BlitzenCore::BlitKey::__ADD;
                case XK_KP_Separator:
                    return BlitzenCore::BlitKey::__SEPARATOR;
                case XK_KP_Subtract:
                    return BlitzenCore::BlitKey::__SUBTRACT;
                case XK_KP_Decimal:
                    return BlitzenCore::BlitKey::__DECIMAL;
                case XK_KP_Divide:
                    return BlitzenCore::BlitKey::__DIVIDE;
                case XK_F1:
                    return BlitzenCore::BlitKey::__F1;
                case XK_F2:
                    return BlitzenCore::BlitKey::__F2;
                case XK_F3:
                    return BlitzenCore::BlitKey::__F3;
                case XK_F4:
                    return BlitzenCore::BlitKey::__F4;
                case XK_F5:
                    return BlitzenCore::BlitKey::__F5;
                case XK_F6:
                    return BlitzenCore::BlitKey::__F6;
                case XK_F7:
                    return BlitzenCore::BlitKey::__F7;
                case XK_F8:
                    return BlitzenCore::BlitKey::__F8;
                case XK_F9:
                    return BlitzenCore::BlitKey::__F9;
                case XK_F10:
                    return BlitzenCore::BlitKey::__F10;
                case XK_F11:
                    return BlitzenCore::BlitKey::__F11;
                case XK_F12:
                    return BlitzenCore::BlitKey::__F12;
                case XK_F13:
                    return BlitzenCore::BlitKey::__F13;
                case XK_F14:
                    return BlitzenCore::BlitKey::__F14;
                case XK_F15:
                    return BlitzenCore::BlitKey::__F15;
                case XK_F16:
                    return BlitzenCore::BlitKey::__F16;
                case XK_F17:
                    return BlitzenCore::BlitKey::__F17;
                case XK_F18:
                    return BlitzenCore::BlitKey::__F18;
                case XK_F19:
                    return BlitzenCore::BlitKey::__F19;
                case XK_F20:
                    return BlitzenCore::BlitKey::__F20;
                case XK_F21:
                    return BlitzenCore::BlitKey::__F21;
                case XK_F22:
                    return BlitzenCore::BlitKey::__F22;
                case XK_F23:
                    return BlitzenCore::BlitKey::__F23;
                case XK_F24:
                    return BlitzenCore::BlitKey::__F24;

                case XK_Num_Lock:
                    return BlitzenCore::BlitKey::__NUMLOCK;
                case XK_Scroll_Lock:
                    return BlitzenCore::BlitKey::__SCROLL;

                case XK_KP_Equal:
                    return BlitzenCore::BlitKey::__NUMPAD_EQUAL;

                case XK_Shift_L:
                    return BlitzenCore::BlitKey::__LSHIFT;
                case XK_Shift_R:
                    return BlitzenCore::BlitKey::__RSHIFT;
                case XK_Control_L:
                    return BlitzenCore::BlitKey::__LCONTROL;
                case XK_Control_R:
                    return BlitzenCore::BlitKey::__RCONTROL;
                /*case XK_Alt_L:
                    return BlitzenCore::BlitKey::__ALT;
                case XK_Alt_R:
                    return KEY_RALT;*/

                case XK_semicolon:
                    return BlitzenCore::BlitKey::__SEMICOLON;
                case XK_plus:
                    return BlitzenCore::BlitKey::__PLUS;
                case XK_comma:
                    return BlitzenCore::BlitKey::__COMMA;
                case XK_minus:
                    return BlitzenCore::BlitKey::__MINUS;
                case XK_period:
                    return BlitzenCore::BlitKey::__PERIOD;
                case XK_slash:
                    return BlitzenCore::BlitKey::__SLASH;
                case XK_grave:
                    return BlitzenCore::BlitKey::__GRAVE;

                case XK_a:
                case XK_A:
                    return BlitzenCore::BlitKey::__A;
                case XK_b:
                case XK_B:
                    return BlitzenCore::BlitKey::__B;
                case XK_c:
                case XK_C:
                    return BlitzenCore::BlitKey::__C;
                case XK_d:
                case XK_D:
                    return BlitzenCore::BlitKey::__D;
                case XK_e:
                case XK_E:
                    return BlitzenCore::BlitKey::__E;
                case XK_f:
                case XK_F:
                    return BlitzenCore::BlitKey::__F;
                case XK_g:
                case XK_G:
                    return BlitzenCore::BlitKey::__G;
                case XK_h:
                case XK_H:
                    return BlitzenCore::BlitKey::__H;
                case XK_i:
                case XK_I:
                    return BlitzenCore::BlitKey::__I;
                case XK_j:
                case XK_J:
                    return BlitzenCore::BlitKey::__J;
                case XK_k:
                case XK_K:
                    return BlitzenCore::BlitKey::__K;
                case XK_l:
                case XK_L:
                    return BlitzenCore::BlitKey::__L;
                case XK_m:
                case XK_M:
                    return BlitzenCore::BlitKey::__M;
                case XK_n:
                case XK_N:
                    return BlitzenCore::BlitKey::__N;
                case XK_o:
                case XK_O:
                    return BlitzenCore::BlitKey::__O;
                case XK_p:
                case XK_P:
                    return BlitzenCore::BlitKey::__P;
                case XK_q:
                case XK_Q:
                    return BlitzenCore::BlitKey::__Q;
                case XK_r:
                case XK_R:
                    return BlitzenCore::BlitKey::__R;
                case XK_s:
                case XK_S:
                    return BlitzenCore::BlitKey::__S;
                case XK_t:
                case XK_T:
                    return BlitzenCore::BlitKey::__T;
                case XK_u:
                case XK_U:
                    return BlitzenCore::BlitKey::__U;
                case XK_v:
                case XK_V:
                    return BlitzenCore::BlitKey::__V;
                case XK_w:
                case XK_W:
                    return BlitzenCore::BlitKey::__W;
                case XK_x:
                case XK_X:
                    return BlitzenCore::BlitKey::__X;
                case XK_y:
                case XK_Y:
                    return BlitzenCore::BlitKey::__Y;
                case XK_z:
                case XK_Z:
                    return BlitzenCore::BlitKey::__Z;

                default:
                    return BlitzenCore::BlitKey::MAX_KEYS;
            }

        }

        /*
            MEMORY
        */
        void* PlatformMalloc(size_t size, uint8_t aligned)
        {
            return malloc(size);
        }

        void PlatformFree(void* pBlock, uint8_t aligned)
        {
            free(pBlock);
        }

        void* PlatformMemZero(void* pBlock, size_t size)
        {
            return memset(pBlock, 0, size);
        }
        void* PlatformMemCopy(void* pDst, void* pSrc, size_t size)
        {
            return memcpy(pDst, pSrc, size);
        }
        void* PlatformMemSet(void* pDst, int32_t value, size_t size)
        {
            return memset(pDst, value, size);
        }

        static void PlatformShutdown(PlatformContext* P_HANDLE)
        {
            // yeah... we got to turn this shit back on, because it's global for the OS
            XAutoRepeatOn(P_HANDLE->m_pDisplay);

            xcb_destroy_window(P_HANDLE->m_pConnection, P_HANDLE->m_window);
        }

        PlatformContext::~PlatformContext()
        {
            PlatformShutdown(this);
        }
}
#endif