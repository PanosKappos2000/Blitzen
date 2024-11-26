#pragma once

#include "platform.h"
#include "Core/blitEvents.h"

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

        struct InternalState
        {
            HINSTANCE winInstance;
            HWND winWindow;
        };

        static double clockFrequency;
        static LARGE_INTEGER startTime;

        LRESULT CALLBACK Win32ProcessMessage(HWND winWindow, uint32_t msg, WPARAM w_param, LPARAM l_param);

        uint8_t PlatformStartup(PlatformState* pState, const char* appName, int32_t x, int32_t y, uint32_t width, uint32_t height)
        {
            pState->internalState = malloc(sizeof(InternalState));
            InternalState* pInternalState = reinterpret_cast<InternalState*>(pState->internalState);

            pInternalState->winInstance = GetModuleHandleA(0);

            HICON icon = LoadIcon(pInternalState->winInstance, IDI_APPLICATION);
            tagWNDCLASSA wc;
            memset(&wc, 0, sizeof(wc));
            wc.style = CS_DBLCLKS;
            // Pass the function that will get called when events get triggered
            wc.lpfnWndProc = Win32ProcessMessage;
            wc.cbClsExtra = 0;
            wc.cbWndExtra = 0;
            wc.hInstance = pInternalState->winInstance;
            wc.hIcon = icon;
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wc.hbrBackground = nullptr;
            // !This must be the exact same as the 2nd parameter of CreateWindowExA
            wc.lpszClassName = "BlitzenWindowClass";

            if(!RegisterClassA(&wc))
            {
                MessageBoxA(pInternalState->winWindow, "Window registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);
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
            pInternalState->winInstance, 0);

            // Only assign the handle if it was actually created, otherwise the application should fail
            if(!handle)
            {
                MessageBoxA(nullptr, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
                BLIT_FATAL("Window creation failed!")
                return 0;
            }
            else
            {
                pInternalState->winWindow = handle;
            }

            //Tell the window to show
            uint8_t shouldActivate = 1;
            int32_t show = shouldActivate ? SW_SHOW : SW_SHOWNOACTIVATE;
            ShowWindow(pInternalState->winWindow, show);

            // Clock setup, similar thing to glfwGetTime
            LARGE_INTEGER frequency;
            QueryPerformanceCounter(&frequency);
            clockFrequency = 1.0 / static_cast<double>(frequency.QuadPart);// The quad part is just a 64 bit integer
            QueryPerformanceCounter(&startTime);

            return 1;
        }

        void PlatformShutdown(PlatformState* pState)
        {
            InternalState* pInternalState = reinterpret_cast<InternalState*>(pState->internalState);
            if(pInternalState->winWindow)
            {
                DestroyWindow(pInternalState->winWindow);
                pInternalState->winWindow = 0;
            }
        }

        uint8_t PlatformPumpMessages(PlatformState* pState)
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
            return static_cast<double>(nowTime.QuadPart * clockFrequency);
        }

        void PlatformSleep(uint64_t ms)
        {
            Sleep(ms);
        }

        void CreateVulkanSurface(PlatformState* pState, VkInstance& instance, VkSurfaceKHR& surface, VkAllocationCallbacks* pAllocator)
        {
            InternalState* pInternalState = reinterpret_cast<InternalState*>(pState->internalState);

            VkWin32SurfaceCreateInfoKHR info = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
            info.hinstance = pInternalState->winInstance;
            info.hwnd = pInternalState->winWindow;
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
                    // TODO: Fire an event for window closing like glfw
                    return 0;
                }

                case WM_DESTROY:
                {
                    PostQuitMessage(0);
                    return 0;
                }
                case WM_SIZE:
                {
                    // Get the updated size.
                    // RECT r;
                    // GetClientRect(hwnd, &r);
                    // u32 width = r.right - r.left;
                    // u32 height = r.bottom - r.top;
                    // TODO: Fire an event for window resize.
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

        struct InternalState
        {
            Display* pDisplay;
            xcb_connection_t* pConnection;
            xcb_window_t window;
            xcb_screen_t* pScreen;
            xcb_atom_t wm_protocols;
            xcb_atom_t wm_delete_win;
        };

        uint8_t PlatformStartup(PlatformState* pState, const char* appName, int32_t x, int32_t y, uint32_t width, uint32_t height)
        {
            pState->internalState = malloc(sizeof(InternalState));
            InternalState* pInternalState = reinterpret_cast<InternalState*>(pState->internalState);
        }

    #endif
}