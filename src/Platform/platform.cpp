#include "platform.h"
#include "Core/blitEvents.h"

// Including Vulkan to load the VkSurfaceKHR since that is platform specific
#include "BlitzenVulkan/vulkanData.h"

// Including the opengl renderer to create the platform specific rendering context
#include "BlitzenGl/openglRenderer.h"

// The Engine class is needed to see if it has been initialized
#include "Engine/blitzenEngine.h"

// Need this for memchr and strchr
#include <cstring>

namespace BlitzenPlatform
{
    /*----------------
        WINDOWS   !
    -----------------*/

#ifdef _WIN32
#include <windows.h>
#include <Windows.h>
#include <windowsx.h>
#include <WinUser.h>
#include <windowsx.h>
#include <vulkan/vulkan_win32.h>
    // Necessary for some wgl function pointers
#include <GL/wglew.h>

    struct PlatformState
    {
        HINSTANCE winInstance;
        HWND winWindow;

        // gl render context
        HGLRC hglrc;
    };

    inline PlatformState inl_pPlatformState;

    void* GetWindowHandle() { return inl_pPlatformState.winWindow; }

    inline double clockFrequency;
    inline LARGE_INTEGER startTime;

    LRESULT CALLBACK Win32ProcessMessage(HWND winWindow, uint32_t msg, WPARAM w_param, LPARAM l_param);

    size_t GetPlatformMemoryRequirements()
    {
        return sizeof(PlatformState);
    }

    uint8_t PlatformStartup(const char* appName)
    {
        // Platform cannot startup if the engine or the event system have not been initialized first
        if (!(BlitzenEngine::Engine::GetEngineInstancePointer()) ||
            !(BlitzenCore::EventSystemState::GetState()) ||
            !(BlitzenCore::InputSystemState::GetState()))
            return 0;

        inl_pPlatformState.winInstance = GetModuleHandleA(0);

        HICON icon = LoadIcon(inl_pPlatformState.winInstance, IDI_APPLICATION);
        tagWNDCLASSA wc;
        memset(&wc, 0, sizeof(wc));
        wc.style = CS_DBLCLKS;
        // Pass the function that will get called when events get triggered
        wc.lpfnWndProc = Win32ProcessMessage;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = inl_pPlatformState.winInstance;
        wc.hIcon = icon;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;

        // This must be the exact same as the 2nd parameter of CreateWindowExA
        wc.lpszClassName = "BlitzenWindowClass";

        if (!RegisterClassA(&wc))
        {
            MessageBoxA(inl_pPlatformState.winWindow, "Window registration failed", "Error",
                MB_ICONEXCLAMATION | MB_OK);
            return 0;
        }

        // Sets the window's client size
        uint32_t clientX = BlitzenEngine::ce_windowStartingX;
        uint32_t clientY = BlitzenEngine::ce_windowStartingY;
        uint32_t clientWidth = BlitzenEngine::ce_initialWindowWidth;
        uint32_t clientHeight = BlitzenEngine::ce_initialWindowHeight;

        // Sets the size of the window to be the same as the client momentarily
        uint32_t windowX = clientX;
        uint32_t windowY = clientY;
        uint32_t windowWidth = clientWidth;
        uint32_t windowHeight = clientHeight;

        uint32_t windowStyle = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
        uint32_t windowExStyle = WS_EX_APPWINDOW;

        windowStyle |= WS_MAXIMIZEBOX;
        windowStyle |= WS_MINIMIZEBOX;
        windowStyle |= WS_THICKFRAME;

        RECT borderRect = { 0, 0, 0, 0 };
        AdjustWindowRectEx(&borderRect, windowStyle, 0, windowExStyle);

        // Sets the true size of the window
        windowX += borderRect.left;
        windowY += borderRect.top;
        windowWidth += borderRect.right - borderRect.left;
        windowHeight -= borderRect.top - borderRect.bottom;

        //Creates the window
        HWND handle = CreateWindowExA(
            windowExStyle, "BlitzenWindowClass", appName,
            windowStyle, windowX, windowY, windowWidth, windowHeight,
            0, 0, inl_pPlatformState.winInstance, 0);

        // Only assign the handle if it was actually created, otherwise the application should fail
        if (!handle)
        {
            MessageBoxA(nullptr, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
            return 0;
        }
        else
        {
            inl_pPlatformState.winWindow = handle;
        }

        // Tells the window to show
        int32_t show = SW_SHOW; //: SW_SHOWNOACTIVATE;
        ShowWindow(inl_pPlatformState.winWindow, show);

        // Clock setup, similar thing to glfwGetTime
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        clockFrequency = 1.0 / static_cast<double>(frequency.QuadPart);// The quad part is just a 64 bit integer
        QueryPerformanceCounter(&startTime);

        return 1;
    }

    void PlatformShutdown()
    {
        // Delete the gl render context
        if (inl_pPlatformState.hglrc)
        {
            wglDeleteContext(inl_pPlatformState.hglrc);
        }

        if (inl_pPlatformState.winWindow)
        {
            DestroyWindow(inl_pPlatformState.winWindow);
        }
    }

    uint8_t PlatformPumpMessages()
    {
        MSG message;
        while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE))
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
        static uint8_t levels[6] = { 64, 4, 6, 2, 1, 8 };
        SetConsoleTextAttribute(consoleHandle, levels[color]);
        OutputDebugStringA(message);
        uint64_t length = strlen(message);
        LPDWORD numberWritten = 0;
        WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message, static_cast<DWORD>(length), numberWritten, 0);
    }

    void PlatformConsoleError(const char* message, uint8_t color)
    {
        HANDLE consoleHandle = GetStdHandle(STD_ERROR_HANDLE);
        static uint8_t levels[6] = { 64, 4, 6, 2, 1, 8 };
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

        uint8_t CreateVulkanSurface(VkInstance& instance, VkSurfaceKHR& surface, VkAllocationCallbacks* pAllocator)
        {
            VkWin32SurfaceCreateInfoKHR info = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
            info.hinstance = inl_pPlatformState.winInstance;
            info.hwnd = inl_pPlatformState.winWindow;

            VkResult res = vkCreateWin32SurfaceKHR(instance, &info, pAllocator, &surface);
            if(res != VK_SUCCESS)
                return 0;
            return 1;
        }

        uint8_t CreateOpenglDrawContext()
        {
            // Get the device context of the window
            HDC hdc = GetDC(inl_pPlatformState.winWindow);

            // Pixel format
            PIXELFORMATDESCRIPTOR pfd;
            pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
            pfd.nVersion = 1;
            pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_SUPPORT_COMPOSITION | PFD_DOUBLEBUFFER;
            pfd.iPixelType = PFD_TYPE_RGBA;
            pfd.cColorBits = 32;
            pfd.cAlphaBits = 8;
            pfd.iLayerType = PFD_MAIN_PLANE;

            // Setup a dummy pixel format 
            int formatIndex = ChoosePixelFormat(hdc, &pfd);
            if(!formatIndex)
            {
                return 0;
            }
            if(!SetPixelFormat(hdc, formatIndex, &pfd))
            {
                return 0;
            }
            if (!DescribePixelFormat(hdc, formatIndex, sizeof(pfd), &pfd))
            {
                return 0;
            }

            if ((pfd.dwFlags & PFD_SUPPORT_OPENGL) != PFD_SUPPORT_OPENGL)
            {
                return 0;
            }

            // Create the dummy render context
            inl_pPlatformState.hglrc = wglCreateContext(hdc);

            // Make this the current context so that glew can be initialized
            if(!wglMakeCurrent(hdc, inl_pPlatformState.hglrc))
            {
                return 0;
            }

            // Initializes glew
            if (glewInit() != GLEW_OK)
            {
                return 0;
            }

            // With glew now available, extension function pointers can be accessed and a better gl context can be retrieved
            // So the old render context is deleted and the device is released
            wglDeleteContext(inl_pPlatformState.hglrc);
            ReleaseDC(inl_pPlatformState.winWindow, hdc);

            // Get a new device context
            hdc = GetDC(inl_pPlatformState.winWindow);

            // Choose a pixel format with attributes
            const int attribList[] =
            {
                WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
                WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
                WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
                WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
                WGL_COLOR_BITS_ARB, 32,
                WGL_DEPTH_BITS_ARB, 24,
                WGL_STENCIL_BITS_ARB, 8,
                0, // End
            };
            int pixelFormat;
            UINT numFormats;
            wglChoosePixelFormatARB(hdc, attribList, NULL, 1, &pixelFormat, &numFormats);

            // Set the pixel format
            PIXELFORMATDESCRIPTOR pixelFormatDesc = {};
	        DescribePixelFormat(hdc, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pixelFormatDesc);
	        SetPixelFormat(hdc, pixelFormat, &pixelFormatDesc);

            // Create a new render context with attributes (latest opengl version)
            int const createAttribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 4, WGL_CONTEXT_MINOR_VERSION_ARB,  6,
		    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0};
            inl_pPlatformState.hglrc = wglCreateContextAttribsARB(hdc, 0, createAttribs);

            // Set the gl render context as the new one
            return (wglMakeCurrent(hdc, inl_pPlatformState.hglrc));
        }

        void OpenglSwapBuffers()
        {
            #ifdef BLIT_VSYNC
                wglSwapIntervalEXT(1);
            #else
                wglSwapIntervalEXT(0);
            #endif
            wglSwapLayerBuffers(GetDC(inl_pPlatformState.winWindow), WGL_SWAP_MAIN_PLANE);
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
                    BlitzenCore::FireEvent<void*>(BlitzenCore::BlitEventType::EngineShutdown, nullptr, context);
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
                    BlitzenCore::FireEvent<void*>(BlitzenCore::BlitEventType::WindowResize, nullptr, context);
                    break;
                }

                // Key event (input system)
                case WM_KEYDOWN:
                case WM_SYSKEYDOWN:
                case WM_KEYUP:
                case WM_SYSKEYUP: 
                {
                    uint8_t pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);// key pressed or released
                    BlitzenCore::BlitKey key = static_cast<BlitzenCore::BlitKey>(w_param); // key code
                    BlitzenCore::InputProcessKey(key, pressed);// let input system know
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
                    uint8_t bPressed = msg == WM_LBUTTONDOWN || 
                        msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN;
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

        #include <vulkan/vulkan_xcb.h>

        struct PlatformState
        {
            Display* pDisplay;
            xcb_connection_t* pConnection;
            xcb_window_t window;
            xcb_screen_t* pScreen;
            xcb_atom_t wm_protocols;
            xcb_atom_t wm_delete_win;
        };

        inline PlatformState s_state;

        // Key translation
        BlitzenCore::BlitKey TranslateKeycode(uint32_t xKeycode);

        uint8_t PlatformStartup(const char* appName)
        {
            s_state.pDisplay = XOpenDisplay(nullptr);

            // Turn off key repeats.
            XAutoRepeatOff(s_state.pDisplay);

            // Retrieve the connection from the display.
            s_state.pConnection = XGetXCBConnection(s_state.pDisplay);

            if (xcb_connection_has_error(s_state.pConnection)) 
            {
                BLIT_FATAL("Failed to connect to X server via XCB.");
                return 0;
            }

            const struct xcb_setup_t* setup = xcb_get_setup(s_state.pConnection);

            // Loop through screens using iterator
            xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
            int screenP = 0;
            for (int32_t s = screenP; s > 0; s--)
            {
                xcb_screen_next(&it);
            }

            // After screens have been looped through, assign it.
            s_state.pScreen = it.data;

            // Allocate a XID for the window to be created.
            s_state.window = xcb_generate_id(s_state.pConnection);

            // Register event types.
            // XCB_CW_BACK_PIXEL = filling then window bg with a single colour
            // XCB_CW_EVENT_MASK is required.
            uint32_t eventMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

            // Listen for keyboard and mouse buttons
            uint32_t eventValues = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
            XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

            // Values to be sent over XCB (bg colour, events)
            uint32_t valueList[] = { s_state.pScreen->black_pixel, eventValues };

            // Create the window
            xcb_void_cookie_t cookie = xcb_create_window(s_state.pConnection,XCB_COPY_FROM_PARENT/* depth */, s_state.window,
            s_state.pScreen->root/* parent */,
            BlitzenEngine::ce_windowStartingX, BlitzenEngine::ce_windowStartingY, 
            BlitzenEngine::ce_initialWindowWidth, BlitzenEngine::ce_initialWindowHeight, 0/* No border */, 
            XCB_WINDOW_CLASS_INPUT_OUTPUT, s_state.pScreen->root_visual, eventMask, valueList);

            // Change the title
            xcb_change_property( s_state.pConnection, XCB_PROP_MODE_REPLACE, s_state.window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING,
            8/* data should be viewed 8 bits at a time */, strlen(appName), appName);

            // Tell the server to notify when the window manager
            // attempts to destroy the window.
            xcb_intern_atom_cookie_t wm_delete_cookie = xcb_intern_atom(s_state.pConnection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
            xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(s_state.pConnection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
            xcb_intern_atom_reply_t* wm_delete_reply = xcb_intern_atom_reply(s_state.pConnection, wm_delete_cookie, nullptr);
            xcb_intern_atom_reply_t* wm_protocols_reply = xcb_intern_atom_reply( s_state.pConnection, wm_protocols_cookie, nullptr);
            s_state.wm_delete_win = wm_delete_reply->atom;
            s_state.wm_protocols = wm_protocols_reply->atom;

            xcb_change_property(s_state.pConnection,XCB_PROP_MODE_REPLACE, s_state.window, wm_protocols_reply->atom, 4, 32, 1, 
            &wm_delete_reply->atom);

            // Map the window to the screen
            xcb_map_window(s_state.pConnection, s_state.window);

            // Flush the stream
            int32_t streamResult = xcb_flush(s_state.pConnection);
            if (streamResult <= 0)
            {
                BLIT_FATAL("An error occurred when flusing the stream: %d", streamResult);
                return 0;
            }

            return 1;
        }

        void PlatformShutdown() 
        {
            // Turn key repeats back on since this is global for the OS... just... wow.
            XAutoRepeatOn(s_state.pDisplay);

            xcb_destroy_window(s_state.pConnection, s_state.window);
        }

        uint8_t PlatformPumpMessages() 
        {
            
                xcb_generic_event_t* event;
                xcb_client_message_event_t* cm;

                uint8_t quitFlagged = 0;

                // Poll for events until null is returned.
                while (event) 
                {
                    event = xcb_poll_for_event(s_state.pConnection);
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
                        KeySym keySym = XkbKeycodeToKeysym(s_state.pDisplay,(KeyCode)code,  //event.xkey.keycode,
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
                        BlitzenCore::MouseButton mouseButton = BlitzenCore::MouseButton::MaxButtons;
                        switch (mouseEvent->detail) {
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

                        // Pass over to the input subsystem.
                        if (mouseButton != BlitzenCore::MouseButton::MaxButtons) {
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
                        BlitzenCore::EventContext context;
                        context.data.ui32[0] = configureEvent->width;
                        context.data.ui32[1] = configureEvent->height;
                        BlitzenCore::FireEvent(BlitzenCore::BlitEventType::WindowResize, 0, context);
                        break;
                    }

                    case XCB_CLIENT_MESSAGE: 
                    {
                        cm = (xcb_client_message_event_t*)event;

                        // Window close
                        if (cm->data.data32[0] == s_state.wm_delete_win) 
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

        uint8_t CreateVulkanSurface(VkInstance& instance, VkSurfaceKHR& surface, VkAllocationCallbacks* pAllocator)
        {
            VkXcbSurfaceCreateInfoKHR info{};
            info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
            info.connection = s_state.pConnection;
            info.window = s_state.window;

            VkResult res = vkCreateXcbSurfaceKHR(instance, &info, pAllocator,&surface);
            if(res != VK_SUCCESS)
                return 0;
            return 1;
        }

        double PlatformGetAbsoluteTime() 
        {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            return now.tv_sec + now.tv_nsec * 0.000000001;
        }

        void platform_sleep(uint64_t ms) 
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


    #endif
}