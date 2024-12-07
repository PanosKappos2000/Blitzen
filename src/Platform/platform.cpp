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

        static PlatformState* s_pPlatformState;

        static double clockFrequency;
        static LARGE_INTEGER startTime;

        LRESULT CALLBACK Win32ProcessMessage(HWND winWindow, uint32_t msg, WPARAM w_param, LPARAM l_param);

        size_t GetPlatformMemoryRequirements()
        {
            return sizeof(PlatformState);
        }

        uint8_t PlatformStartup(void* pState, const char* appName, int32_t x, int32_t y, uint32_t width, uint32_t height)
        {
            s_pPlatformState = reinterpret_cast<PlatformState*>(pState);

            s_pPlatformState->winInstance = GetModuleHandleA(0);

            HICON icon = LoadIcon(s_pPlatformState->winInstance, IDI_APPLICATION);
            tagWNDCLASSA wc;
            memset(&wc, 0, sizeof(wc));
            wc.style = CS_DBLCLKS;
            // Pass the function that will get called when events get triggered
            wc.lpfnWndProc = Win32ProcessMessage;
            wc.cbClsExtra = 0;
            wc.cbWndExtra = 0;
            wc.hInstance = s_pPlatformState->winInstance;
            wc.hIcon = icon;
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wc.hbrBackground = nullptr;
            // !This must be the exact same as the 2nd parameter of CreateWindowExA
            wc.lpszClassName = "BlitzenWindowClass";

            if(!RegisterClassA(&wc))
            {
                MessageBoxA(s_pPlatformState->winWindow, "Window registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);
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
            s_pPlatformState->winInstance, 0);

            // Only assign the handle if it was actually created, otherwise the application should fail
            if(!handle)
            {
                MessageBoxA(nullptr, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
                BLIT_FATAL("Window creation failed!")
                return 0;
            }
            else
            {
                s_pPlatformState->winWindow = handle;
            }

            //Tell the window to show
            uint8_t shouldActivate = 1;
            int32_t show = shouldActivate ? SW_SHOW : SW_SHOWNOACTIVATE;
            ShowWindow(s_pPlatformState->winWindow, show);

            // Clock setup, similar thing to glfwGetTime
            LARGE_INTEGER frequency;
            QueryPerformanceFrequency(&frequency);
            clockFrequency = 1.0 / static_cast<double>(frequency.QuadPart);// The quad part is just a 64 bit integer
            QueryPerformanceCounter(&startTime);

            return 1;
        }

        void PlatformShutdown()
        {
            if(s_pPlatformState->winWindow)
            {
                DestroyWindow(s_pPlatformState->winWindow);
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
            Sleep(ms);
        }

        void CreateVulkanSurface(VkInstance& instance, VkSurfaceKHR& surface, VkAllocationCallbacks* pAllocator)
        {
            VkWin32SurfaceCreateInfoKHR info = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
            info.hinstance = s_pPlatformState->winInstance;
            info.hwnd = s_pPlatformState->winWindow;
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

        static PlatformState* s_pState;

        // Key translation
        BlitzenCore::BlitKey TranslateKeycode(uint32_t xKeycode);

        uint8_t PlatformStartup(void* pState, const char* appName, int32_t x, int32_t y, uint32_t width, uint32_t height)
        {
            s_pState = reinterpret_cast<PlatformState*>(pState);

            pState->pDisplay = XOpenDisplay(nullptr);

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



    #endif
}