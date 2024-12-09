#pragma once

#include "platform.h"
#include "Core/blitEvents.h"
#include "BlitzenVulkan/vulkanData.h"

namespace BlitzenPlatform
{
    /*----------------
        WINDOWS   !
    -----------------*/

    #if _MSC_VER
        #include <windows.h>
        #include <windowsx.h>
        #include <WinUser.h>
        #include "windowsx.h"
        #include "vulkan/vulkan_win32.h"

        struct PlatformState
        {
            HINSTANCE winInstance;
            HWND winWindow;
        };    

        static PlatformState s_pPlatformState;

        static double clockFrequency;
        static LARGE_INTEGER startTime;

        LRESULT CALLBACK Win32ProcessMessage(HWND winWindow, uint32_t msg, WPARAM w_param, LPARAM l_param);

        size_t GetPlatformMemoryRequirements()
        {
            return sizeof(PlatformState);
        }

        uint8_t PlatformStartup(const char* appName, int32_t x, int32_t y, uint32_t width, uint32_t height)
        {
            s_pPlatformState.winInstance = GetModuleHandleA(0);

            HICON icon = LoadIcon(s_pPlatformState.winInstance, IDI_APPLICATION);
            tagWNDCLASSA wc;
            memset(&wc, 0, sizeof(wc));
            wc.style = CS_DBLCLKS;
            // Pass the function that will get called when events get triggered
            wc.lpfnWndProc = Win32ProcessMessage;
            wc.cbClsExtra = 0;
            wc.cbWndExtra = 0;
            wc.hInstance = s_pPlatformState.winInstance;
            wc.hIcon = icon;
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wc.hbrBackground = nullptr;
            // !This must be the exact same as the 2nd parameter of CreateWindowExA
            wc.lpszClassName = "BlitzenWindowClass";

            if(!RegisterClassA(&wc))
            {
                MessageBoxA(s_pPlatformState.winWindow, "Window registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);
                return 0;
            }

            // Set the window's client size
            uint32_t clientX = x;
            uint32_t clientY = y;
            uint32_t clientWidth = width;
            uint32_t clientHeight = height;

            // Set the size of the window to be the same as the client momentarily
            uint32_t windowX = clientX;
            uint32_t windowY = clientY;
            uint32_t windowWidth = clientWidth;
            uint32_t windowHeight = clientHeight;

            uint32_t windowStyle = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
            uint32_t windowExStyle = WS_EX_APPWINDOW;

            windowStyle |= WS_MAXIMIZEBOX;
            windowStyle |= WS_MINIMIZEBOX;
            windowStyle |= WS_THICKFRAME;

            RECT borderRect = {0, 0, 0, 0};
            AdjustWindowRectEx(&borderRect, windowStyle, 0, windowExStyle);

            // Set the true size of the window
            windowX += borderRect.left;
            windowY += borderRect.top;
            windowWidth += borderRect.right - borderRect.left;
            windowHeight -= borderRect.top - borderRect.bottom;

            //Create the window
            HWND handle = CreateWindowExA(windowExStyle, "BlitzenWindowClass", appName, windowStyle, windowX, windowY, windowWidth, windowHeight, 0, 0,
            s_pPlatformState.winInstance, 0);

            // Only assign the handle if it was actually created, otherwise the application should fail
            if(!handle)
            {
                MessageBoxA(nullptr, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
                BLIT_FATAL("Window creation failed!")
                return 0;
            }
            else
            {
                s_pPlatformState.winWindow = handle;
            }

            //Tell the window to show
            uint8_t shouldActivate = 1;
            int32_t show = shouldActivate ? SW_SHOW : SW_SHOWNOACTIVATE;
            ShowWindow(s_pPlatformState.winWindow, show);

            // Clock setup, similar thing to glfwGetTime
            LARGE_INTEGER frequency;
            QueryPerformanceFrequency(&frequency);
            clockFrequency = 1.0 / static_cast<double>(frequency.QuadPart);// The quad part is just a 64 bit integer
            QueryPerformanceCounter(&startTime);

            return 1;
        }

        void PlatformShutdown()
        {
            if(s_pPlatformState.winWindow)
            {
                DestroyWindow(s_pPlatformState.winWindow);
            }
        }

        uint8_t PlatformPumpMessages()
        {
            MSG message;
            while(PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
            }

            return 1;
        }

        void* PlatformMalloc(size_t size, uint8_t aligned)
        {
            // temporary
            return malloc(size);
        }

        void PlatformFree(void* pBlock, uint8_t aligned)
        {
            // temporary
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

        void PlatformConsoleWrite(const char* message, uint8_t color)
        {
            HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
            static uint8_t levels[6] = {64, 4, 6, 2, 1, 8};
            SetConsoleTextAttribute(consoleHandle, levels[color]);
            OutputDebugStringA(message);
            uint64_t length = strlen(message);
            LPDWORD numberWritten = 0;
            WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message, static_cast<DWORD>(length), numberWritten, 0);
        }

        void PlatformConsoleError(const char* message, uint8_t color)
        {
            HANDLE consoleHandle = GetStdHandle(STD_ERROR_HANDLE);
            static uint8_t levels[6] = {64, 4, 6, 2, 1, 8};
            SetConsoleTextAttribute(consoleHandle, levels[color]);
            OutputDebugStringA(message);
            uint64_t length = strlen(message);
            LPDWORD numberWritten = 0;
            WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), message, static_cast<DWORD>(length), numberWritten, 0);
        }

        double PlatformGetAbsoluteTime()
        {
            LARGE_INTEGER nowTime;
            QueryPerformanceCounter(&nowTime);
            return static_cast<double>(nowTime.QuadPart) * clockFrequency;
        }

        void PlatformSleep(uint64_t ms)
        {
            Sleep(static_cast<DWORD>(ms));
        }

        void CreateVulkanSurface(VkInstance& instance, VkSurfaceKHR& surface, VkAllocationCallbacks* pAllocator)
        {
            VkWin32SurfaceCreateInfoKHR info = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
            info.hinstance = s_pPlatformState.winInstance;
            info.hwnd = s_pPlatformState.winWindow;
            vkCreateWin32SurfaceKHR(instance, &info, pAllocator, &surface);
        }




        LRESULT CALLBACK Win32ProcessMessage(HWND winWindow, uint32_t msg, WPARAM w_param, LPARAM l_param)
        {
            switch(msg)
            {
                case WM_ERASEBKGND:
                {
                    // Notify the OS that erasing will be handled by the application to prevent flicker
                    return 1;
                }

                case WM_CLOSE:
                {
                    BlitzenCore::EventContext context{};
                    BlitzenCore::FireEvent(BlitzenCore::BlitEventType::EngineShutdown, nullptr, context);
                    return 1;
                }

                case WM_DESTROY:
                {
                    PostQuitMessage(0);
                    return 0;
                }
                case WM_SIZE:
                {
                    // Get the updated size.
                    RECT rect;
                    GetClientRect(winWindow, &rect);
                    uint32_t width = rect.right - rect.left;
                    uint32_t height = rect.bottom - rect.top;
                    BlitzenCore::EventContext context;
                    context.data.ui32[0] = width;
                    context.data.ui32[1] = height;
                    BlitzenCore::FireEvent(BlitzenCore::BlitEventType::WindowResize, nullptr, context);
                    break;
                }
                case WM_KEYDOWN:
                case WM_SYSKEYDOWN:
                case WM_KEYUP:
                case WM_SYSKEYUP: 
                {
                    // Key pressed/released
                    uint8_t pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
                    BlitzenCore::BlitKey key = static_cast<BlitzenCore::BlitKey>(w_param);
                    BlitzenCore::InputProcessKey(key, pressed);
                    break;
                } 
                case WM_MOUSEMOVE: 
                {
                    // Mouse move
                    int32_t mouseX = GET_X_LPARAM(l_param);
                    int32_t mouseY = GET_Y_LPARAM(l_param);
                    BlitzenCore::InputProcessMouseMove(mouseX, mouseY);
                    break;
                } 
                case WM_MOUSEWHEEL: 
                {
                    int32_t zDelta = GET_WHEEL_DELTA_WPARAM(w_param);
                    if (zDelta != 0) 
                    {
                        // Flatten the input to an OS-independent (-1, 1)
                        zDelta = (zDelta < 0) ? -1 : 1;
                        BlitzenCore::InputProcessMouseWheel(zDelta);
                        break;
                    }
                }
                case WM_LBUTTONDOWN:
                case WM_MBUTTONDOWN:
                case WM_RBUTTONDOWN:
                case WM_LBUTTONUP:
                case WM_MBUTTONUP:
                case WM_RBUTTONUP: 
                {
                    uint8_t bPressed = (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN);
                    BlitzenCore::MouseButton button = BlitzenCore::MouseButton::MaxButtons;
                    switch(msg)
                    {
                        case WM_LBUTTONDOWN:
                        case WM_LBUTTONUP:
                        {
                            button = BlitzenCore::MouseButton::Left;
                            break;
                        }
                        case WM_RBUTTONDOWN:
                        case WM_RBUTTONUP:
                        {
                            button = BlitzenCore::MouseButton::Right;
                            break;
                        }
                        case WM_MBUTTONDOWN:
                        case WM_MBUTTONUP:
                        {
                            button = BlitzenCore::MouseButton::Middle;
                            break;
                        }
                    }
                    if(button != BlitzenCore::MouseButton::MaxButtons)
                        BlitzenCore::InputProcessButton(button, bPressed);
                    break;
                } 
            }
            // For any events that were not included above, windows can go ahead and handle them like it normally would
            return DefWindowProcA(winWindow, msg, w_param, l_param); 
        }
    #endif


            /*--------------
                LINUX  ! 
            ---------------*/



    #ifdef linux

        #include <xcb/xcb.h>
        #include <X11/keysym.h>
        #include <X11/XKBlib.h>  // sudo apt-get install libx11-dev
        #include <X11/Xlib.h>
        #include <X11/Xlib-xcb.h>  // sudo apt-get install libxkbcommon-x11-dev
        #include <sys/time.h>

        #if _POSIX_C_SOURCE >= 199309L
        #include <time.h>  // nanosleep
        #else
        #include <unistd.h>  // usleep
        #endif

        #include <stdlib.h>
        #include <stdio.h>
        #include <string.h>

        struct PlatformState
        {
            Display* pDisplay;
            xcb_connection_t* pConnection;
            xcb_window_t window;
            xcb_screen_t* pScreen;
            xcb_atom_t wm_protocols;
            xcb_atom_t wm_delete_win;
        };

        static PlatformState s_pState;

        // Key translation
        BlitzenCore::BlitKey TranslateKeycode(uint32_t xKeycode);

        uint8_t PlatformStartup(const char* appName, int32_t x, int32_t y, uint32_t width, uint32_t height)
        {
            pState.pDisplay = XOpenDisplay(nullptr);

            // Turn off key repeats.
            XAutoRepeatOff(s_pState->pDisplay);

            // Retrieve the connection from the display.
            pState->pConnection = XGetXCBConnection(pState->pDisplay);

            if (xcb_connection_has_error(pState->pConnection)) 
            {
                BLIT_FATAL("Failed to connect to X server via XCB.");
                return 0;
            }

            const struct xcb_setup_t* setup = xcb_get_setup(pState->pConnection);

            // Loop through screens using iterator
            xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
            int screenP = 0;
            for (int32_t s = screenP; s > 0; s--)
            {
                xcb_screen_next(&it);
            }

            // After screens have been looped through, assign it.
            pState->pScreen = it.data;

            // Allocate a XID for the window to be created.
            pState->window = xcb_generate_id(pState->pConnection);

            // Register event types.
            // XCB_CW_BACK_PIXEL = filling then window bg with a single colour
            // XCB_CW_EVENT_MASK is required.
            uin32_t eventMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

            // Listen for keyboard and mouse buttons
            uint32_t eventValues = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
            XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

            // Values to be sent over XCB (bg colour, events)
            uint32_t valueList[] = { pState->pScreen->black_pixel, eventValues };

            // Create the window
            xcb_void_cookie_t cookie = xcb_create_window(pState->pconnection,XCB_COPY_FROM_PARENT/* depth */, pState->window,
            pState->pScreen->root/* parent */,x, y, width, height, 0/* No border */, XCB_WINDOW_CLASS_INPUT_OUTPUT,
            pState->pScreen->root_visual, eventMask, valueList);

            // Change the title
            xcb_change_property( pState->pConnection, XCB_PROP_MODE_REPLACE, pState->window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING,
            8/* data should be viewed 8 bits at a time */, strlen(appName), appName);

            // Tell the server to notify when the window manager
            // attempts to destroy the window.
            xcb_intern_atom_cookie_t wm_delete_cookie = xcb_intern_atom(pState->pConnection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
            xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(pState->pConnection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
            xcb_intern_atom_reply_t* wm_delete_reply = xcb_intern_atom_reply(pState->pConnection, wm_delete_cookie, nullptr);
            xcb_intern_atom_reply_t* wm_protocols_reply = xcb_intern_atom_reply( pState->pConnection, wm_protocols_cookie, nullptr);
            state_ptr->wm_delete_win = wm_delete_reply->atom;
            state_ptr->wm_protocols = wm_protocols_reply->atom;

            xcb_change_property(pState->pConnection,XCB_PROP_MODE_REPLACE, pState->window, wm_protocols_reply->atom, 4, 32, 1, 
            &wm_delete_reply->atom);

            // Map the window to the screen
            xcb_map_window(state_ptr->connection, state_ptr->window);

            // Flush the stream
            int32_t streamResult = xcb_flush(pState->pConnection);
            if (streamResult <= 0)
            {
                BLIT_FATAL("An error occurred when flusing the stream: %d", streamResult);
                return 0;
            }

            return 1;
        }

        void platform_system_shutdown() 
        {
            if (pState) 
            {
                // Turn key repeats back on since this is global for the OS... just... wow.
                XAutoRepeatOn(pState->display);

                xcb_destroy_window(pState->connection, pState->window);
            }
        }

        uint8_t PlatformPumpMessages() 
        {
            if (pState) 
            {
                xcb_generic_event_t* event;
                xcb_client_message_event_t* cm;

                uint8_t quitFlagged = 0;

                // Poll for events until null is returned.
                while (event) 
                {
                    event = xcb_poll_for_event(pState->pConnection);
                    if (!event) 
                    {
                        break;
                    }

                    // Input events
                    switch (event->response_type & ~0x80) {
                    case XCB_KEY_PRESS:
                    case XCB_KEY_RELEASE: 
                    {
                        // Key press event - xcb_key_press_event_t and xcb_key_release_event_t are the same
                        xcb_key_press_event_t* kb_event = (xcb_key_press_event_t*)event;
                        uint8_t pressed = event->response_type == XCB_KEY_PRESS;
                        xcb_keycode_t code = kb_event->detail;
                        KeySym keySym = XkbKeycodeToKeysym(pState->pDisplay,(KeyCode)code,  //event.xkey.keycode,
                        0, code & ShiftMask ? 1 : 0);

                        BlitzenCore::BlitKey key = TranslateKeycode(keySym);

                        // Pass to the input subsystem for processing.
                        BlitzenCore::InputProcessKey(key, pressed);
                    } break;
                    case XCB_BUTTON_PRESS:
                    case XCB_BUTTON_RELEASE: 
                    {
                        xcb_button_press_event_t* mouseEvent = (xcb_button_press_event_t*)event;
                        uint8_t bPressed = event->response_type == XCB_BUTTON_PRESS;
                        buttons mouseButton = BUTTON_MAX_BUTTONS;
                        switch (mouseEvent->detail) {
                        case XCB_BUTTON_INDEX_1:
                            mouseButton = BUTTON_LEFT;
                            break;
                        case XCB_BUTTON_INDEX_2:
                            mouseButton = BUTTON_MIDDLE;
                            break;
                        case XCB_BUTTON_INDEX_3:
                            mouseButton = BUTTON_RIGHT;
                            break;
                        }

                        // Pass over to the input subsystem.
                        if (mouseButton != BUTTON_MAX_BUTTONS) {
                            BlitzenCore::InputProcessButton(mouseButton, bPressed);
                        }
                    } break;
                    case XCB_MOTION_NOTIFY: {
                        // Mouse move
                        xcb_motion_notify_event_t* moveEvent = (xcb_motion_notify_event_t*)event;

                        // Pass over to the input subsystem.
                        BlitzenCore::InputProcessMouseMove(moveEvent->event_x, moveEvent->event_y);
                    } break;
                    case XCB_CONFIGURE_NOTIFY: {
                        // Resizing - note that this is also triggered by moving the window, but should be
                        // passed anyway since a change in the x/y could mean an upper-left resize.
                        // The application layer can decide what to do with this.
                        xcb_configure_notify_event_t* configureEvent = (xcb_configure_notify_event_t*)event;

                        // Fire the event. The application layer should pick this up, but not handle it
                        // as it shouldn be visible to other parts of the application.
                        event_context context;
                        context.data.ui16[0] = configureEvent->width;
                        context.data.ui16[1] = configureEvent->height;
                        BlitzenCore::FireEvent(BlitzenCore::EventType::WindowResize, 0, context);
                        break;
                    }

                    case XCB_CLIENT_MESSAGE: 
                    {
                        cm = (xcb_client_message_event_t*)event;

                        // Window close
                        if (cm->data.data32[0] == pState->wm_delete_win) 
                        {
                            quitFlagged = true;
                        }
                        break;
                    }
                    default:
                        // Something else
                        break;
                    }

                    free(event);
                }
                return !quitFlagged;
            }
            return 1;
        }

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
        void* PlatformMemCopy(void* pDst, void* pSrc, size_t size);
        {
            return memcpy(pDst, pSrc, size);
        }
        void* PlatformMemSet(void* pDst, int32_t value, size_t size);
        {
            return memset(pDst, value, size);
        }

        void PlatformConsoleWrite(const char* message, uint8_t color)
        {
            // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
            const char* colorStrings[] = { "0;41", "1;31", "1;33", "1;32", "1;34", "1;30" };
            printf("\033[%sm%s\033[0m", colour_strings[colour], message);
        }
        void PlatformConsoleError(const char* message, uint8_t color)
        {
            // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
            const char* colorStrings[] = { "0;41", "1;31", "1;33", "1;32", "1;34", "1;30" };
            printf("\033[%sm%s\033[0m", colorStrings[color], message);
        }

        void CreateVulkanSurface(VkInstance& instance, VkSurfaceKHR& surface, VkAllocationCallbacks* pAllocator)
        {
            if (!pState) 
            {
                return;
            }

            VkXcbSurfaceCreateInfoKHR info{};
            info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
            info.connection = pState->pConnection;
            info.window = pState->window;

            VK_CHECK(vkCreateXcbSurfaceKHR(instance, &info, pAllocator,&surface));
        }

        double PlatformGetAbsoluteTime() 
        {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            return now.tv_sec + now.tv_nsec * 0.000000001;
        }

        void platform_sleep(u64 ms) 
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


        BlitzenCore::BlitKey TranslateKeyCode(uint32_t x_keycode)
        {
            switch (x_keycode)
            {
                case XK_BackSpace:
                    return KEY_BACKSPACE;
                case XK_Return:
                    return KEY_ENTER;
                case XK_Tab:
                    return KEY_TAB;
                    //case XK_Shift: return KEY_SHIFT;
                    //case XK_Control: return KEY_CONTROL;

                case XK_Pause:
                    return KEY_PAUSE;
                case XK_Caps_Lock:
                    return KEY_CAPITAL;

                case XK_Escape:
                    return KEY_ESCAPE;

                    // Not supported
                    // case : return KEY_CONVERT;
                    // case : return KEY_NONCONVERT;
                    // case : return KEY_ACCEPT;

                case XK_Mode_switch:
                    return KEY_MODECHANGE;

                case XK_space:
                    return KEY_SPACE;
                case XK_Prior:
                    return KEY_PRIOR;
                case XK_Next:
                    return KEY_NEXT;
                case XK_End:
                    return KEY_END;
                case XK_Home:
                    return KEY_HOME;
                case XK_Left:
                    return KEY_LEFT;
                case XK_Up:
                    return KEY_UP;
                case XK_Right:
                    return KEY_RIGHT;
                case XK_Down:
                    return KEY_DOWN;
                case XK_Select:
                    return KEY_SELECT;
                case XK_Print:
                    return KEY_PRINT;
                case XK_Execute:
                    return KEY_EXECUTE;
                    // case XK_snapshot: return KEY_SNAPSHOT; // not supported
                case XK_Insert:
                    return KEY_INSERT;
                case XK_Delete:
                    return KEY_DELETE;
                case XK_Help:
                    return KEY_HELP;

                case XK_Meta_L:
                    return KEY_LWIN;  // TODO: not sure this is right
                case XK_Meta_R:
                    return KEY_RWIN;
                    // case XK_apps: return KEY_APPS; // not supported

                    // case XK_sleep: return KEY_SLEEP; //not supported

                case XK_KP_0:
                    return KEY_NUMPAD0;
                case XK_KP_1:
                    return KEY_NUMPAD1;
                case XK_KP_2:
                    return KEY_NUMPAD2;
                case XK_KP_3:
                    return KEY_NUMPAD3;
                case XK_KP_4:
                    return KEY_NUMPAD4;
                case XK_KP_5:
                    return KEY_NUMPAD5;
                case XK_KP_6:
                    return KEY_NUMPAD6;
                case XK_KP_7:
                    return KEY_NUMPAD7;
                case XK_KP_8:
                    return KEY_NUMPAD8;
                case XK_KP_9:
                    return KEY_NUMPAD9;
                case XK_multiply:
                    return KEY_MULTIPLY;
                case XK_KP_Add:
                    return KEY_ADD;
                case XK_KP_Separator:
                    return KEY_SEPARATOR;
                case XK_KP_Subtract:
                    return KEY_SUBTRACT;
                case XK_KP_Decimal:
                    return KEY_DECIMAL;
                case XK_KP_Divide:
                    return KEY_DIVIDE;
                case XK_F1:
                    return KEY_F1;
                case XK_F2:
                    return KEY_F2;
                case XK_F3:
                    return KEY_F3;
                case XK_F4:
                    return KEY_F4;
                case XK_F5:
                    return KEY_F5;
                case XK_F6:
                    return KEY_F6;
                case XK_F7:
                    return KEY_F7;
                case XK_F8:
                    return KEY_F8;
                case XK_F9:
                    return KEY_F9;
                case XK_F10:
                    return KEY_F10;
                case XK_F11:
                    return KEY_F11;
                case XK_F12:
                    return KEY_F12;
                case XK_F13:
                    return KEY_F13;
                case XK_F14:
                    return KEY_F14;
                case XK_F15:
                    return KEY_F15;
                case XK_F16:
                    return KEY_F16;
                case XK_F17:
                    return KEY_F17;
                case XK_F18:
                    return KEY_F18;
                case XK_F19:
                    return KEY_F19;
                case XK_F20:
                    return KEY_F20;
                case XK_F21:
                    return KEY_F21;
                case XK_F22:
                    return KEY_F22;
                case XK_F23:
                    return KEY_F23;
                case XK_F24:
                    return KEY_F24;

                case XK_Num_Lock:
                    return KEY_NUMLOCK;
                case XK_Scroll_Lock:
                    return KEY_SCROLL;

                case XK_KP_Equal:
                    return KEY_NUMPAD_EQUAL;

                case XK_Shift_L:
                    return KEY_LSHIFT;
                case XK_Shift_R:
                    return KEY_RSHIFT;
                case XK_Control_L:
                    return KEY_LCONTROL;
                case XK_Control_R:
                    return KEY_RCONTROL;
                case XK_Alt_L:
                    return KEY_LALT;
                case XK_Alt_R:
                    return KEY_RALT;

                case XK_semicolon:
                    return KEY_SEMICOLON;
                case XK_plus:
                    return KEY_PLUS;
                case XK_comma:
                    return KEY_COMMA;
                case XK_minus:
                    return KEY_MINUS;
                case XK_period:
                    return KEY_PERIOD;
                case XK_slash:
                    return KEY_SLASH;
                case XK_grave:
                    return KEY_GRAVE;

                case XK_a:
                case XK_A:
                    return KEY_A;
                case XK_b:
                case XK_B:
                    return KEY_B;
                case XK_c:
                case XK_C:
                    return KEY_C;
                case XK_d:
                case XK_D:
                    return KEY_D;
                case XK_e:
                case XK_E:
                    return KEY_E;
                case XK_f:
                case XK_F:
                    return KEY_F;
                case XK_g:
                case XK_G:
                    return KEY_G;
                case XK_h:
                case XK_H:
                    return KEY_H;
                case XK_i:
                case XK_I:
                    return KEY_I;
                case XK_j:
                case XK_J:
                    return KEY_J;
                case XK_k:
                case XK_K:
                    return KEY_K;
                case XK_l:
                case XK_L:
                    return KEY_L;
                case XK_m:
                case XK_M:
                    return KEY_M;
                case XK_n:
                case XK_N:
                    return KEY_N;
                case XK_o:
                case XK_O:
                    return KEY_O;
                case XK_p:
                case XK_P:
                    return KEY_P;
                case XK_q:
                case XK_Q:
                    return KEY_Q;
                case XK_r:
                case XK_R:
                    return KEY_R;
                case XK_s:
                case XK_S:
                    return KEY_S;
                case XK_t:
                case XK_T:
                    return KEY_T;
                case XK_u:
                case XK_U:
                    return KEY_U;
                case XK_v:
                case XK_V:
                    return KEY_V;
                case XK_w:
                case XK_W:
                    return KEY_W;
                case XK_x:
                case XK_X:
                    return KEY_X;
                case XK_y:
                case XK_Y:
                    return KEY_Y;
                case XK_z:
                case XK_Z:
                    return KEY_Z;

                default:
                    return 0;
            }

        }


    #endif
}