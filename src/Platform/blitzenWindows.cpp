#if defined(_WIN32)
#include <cstring>
#include <windows.h>
#include <windowsx.h>
#include <WinUser.h>
#include "BlitzenVulkan/vulkanData.h"
#include <vulkan/vulkan_win32.h>
#include "BlitzenGl/openglRenderer.h"
#include <GL/wglew.h>
#include "platform.h"
#include "Core/blitInput.h"
#include "Engine/blitzenEngine.h"

namespace BlitzenPlatform
{

    struct PlatformState
    {
        HINSTANCE winInstance;
        HWND winWindow;

        // gl render context
        HGLRC hglrc;

        BlitzenCore::EventSystemState* pEvents;
        BlitzenCore::InputSystemState* pInputs;
    };

    inline PlatformState inl_pPlatformState;

    void* GetWindowHandle() { return inl_pPlatformState.winWindow; }

    inline double inl_clockFrequency;
    inline LARGE_INTEGER inl_startTime;

    // This is the callback function, for things like window events and key presses
    // Exists only to be passed to lpfnWndProc during startup
    LRESULT CALLBACK Win32ProcessMessage(HWND winWindow, uint32_t msg, WPARAM w_param, LPARAM l_param);

    size_t GetPlatformMemoryRequirements()
    {
        return sizeof(PlatformState);
    }

    bool PlatformStartup(const char* appName, BlitzenCore::EventSystemState* pEvents, 
        BlitzenCore::InputSystemState* pInputs)
    {
        // Platform cannot startup if the engine or the event system have not been initialized first
        if (!BlitzenEngine::Engine::GetEngineInstancePointer())
            return 0;

        inl_pPlatformState.pEvents = pEvents;
        inl_pPlatformState.pInputs = pInputs;

        inl_pPlatformState.winInstance = GetModuleHandleA(0);

        auto icon = LoadIcon(inl_pPlatformState.winInstance, IDI_APPLICATION);
        tagWNDCLASSA wc;
        memset(&wc, 0, sizeof(wc));
        wc.style = CS_DBLCLKS;
        wc.lpfnWndProc = Win32ProcessMessage;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = inl_pPlatformState.winInstance;
        wc.hIcon = icon;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = "BlitzenWindowClass";

        if (!RegisterClassA(&wc))
        {
            MessageBoxA(inl_pPlatformState.winWindow, "Window registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);
            return false;
        }

        // Window size initial
        auto clientX = BlitzenEngine::ce_windowStartingX;
        auto clientY = BlitzenEngine::ce_windowStartingY;
        auto clientWidth = BlitzenEngine::ce_initialWindowWidth;
        auto clientHeight = BlitzenEngine::ce_initialWindowHeight;
        auto windowX = clientX;
        auto windowY = clientY;
        auto windowWidth = clientWidth;
        auto windowHeight = clientHeight;
        auto windowStyle = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
        auto windowExStyle = WS_EX_APPWINDOW;

        windowStyle |= WS_MAXIMIZEBOX;
        windowStyle |= WS_MINIMIZEBOX;
        windowStyle |= WS_THICKFRAME;

        // Window size with border
        RECT borderRect = { 0, 0, 0, 0 };
        AdjustWindowRectEx(&borderRect, windowStyle, 0, windowExStyle);
        windowX += borderRect.left;
        windowY += borderRect.top;
        windowWidth += borderRect.right - borderRect.left;
        windowHeight -= borderRect.top - borderRect.bottom;

        //Creates the window
        auto handle = CreateWindowExA(windowExStyle, "BlitzenWindowClass", appName,
            windowStyle, windowX, windowY, windowWidth, windowHeight,
            0, 0, inl_pPlatformState.winInstance, 0);
        if (!handle)
        {
            MessageBoxA(nullptr, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
            return false;
        }
        else
        {
            inl_pPlatformState.winWindow = handle;
        }
        int32_t show = SW_SHOW; //: SW_SHOWNOACTIVATE;
        ShowWindow(inl_pPlatformState.winWindow, show);

        return true;
    }

    void PlatfrormSetupClock()
    {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        inl_clockFrequency = 1.0 / static_cast<double>(frequency.QuadPart);
        QueryPerformanceCounter(&inl_startTime);
    }

    double PlatformGetAbsoluteTime()
    {
        LARGE_INTEGER nowTime;
        QueryPerformanceCounter(&nowTime);
        return static_cast<double>(nowTime.QuadPart) * inl_clockFrequency;
    }

    void PlatformShutdown()
    {
        if (inl_pPlatformState.hglrc)
        {
            wglDeleteContext(inl_pPlatformState.hglrc);
        }

        if (inl_pPlatformState.winWindow)
        {
            DestroyWindow(inl_pPlatformState.winWindow);
        }
    }

    bool PlatformPumpMessages()
    {
        MSG message;
        while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        return true;
    }

    static void SBlitWinLogSetup(const char* message, uint8_t color, HANDLE consoleHandle)
    {
        static uint8_t levels[6] = { 64, 4, 6, 2, 1, 8 };
        SetConsoleTextAttribute(consoleHandle, levels[color]);
        OutputDebugStringA(message);
        uint64_t length = strlen(message);
        LPDWORD numberWritten = 0;
        WriteConsoleA(consoleHandle, message, static_cast<DWORD>(length), numberWritten, 0);
    }

    void PlatformConsoleWrite(const char* message, uint8_t color)
    {
        auto consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        SBlitWinLogSetup(message, color, consoleHandle);
    }

    void PlatformConsoleError(const char* message, uint8_t color)
    {
        auto consoleHandle = GetStdHandle(STD_ERROR_HANDLE);
        SBlitWinLogSetup(message, color, consoleHandle);
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

        auto res = vkCreateWin32SurfaceKHR(instance, &info, pAllocator, &surface);
        if(res != VK_SUCCESS)
            return 0;
        return 1;
    }

    uint8_t CreateOpenglDrawContext()
    {
        // Get the device context of the window
        auto hdc = GetDC(inl_pPlatformState.winWindow);

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
                inl_pPlatformState.pEvents->FireEvent(BlitzenCore::BlitEventType::EngineShutdown, 
                    nullptr, context);
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
			    inl_pPlatformState.pEvents->FireEvent(BlitzenCore::BlitEventType::WindowResize, nullptr, context);
                break;
            }

            // Key event (input system)
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYUP: 
            {
                uint8_t pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
                BlitzenCore::BlitKey key = static_cast<BlitzenCore::BlitKey>(w_param);
                inl_pPlatformState.pInputs->InputProcessKey(key, pressed);
                break;
            } 
            case WM_MOUSEMOVE: 
            {
                // Mouse move
                int32_t mouseX = GET_X_LPARAM(l_param);
                int32_t mouseY = GET_Y_LPARAM(l_param);
                inl_pPlatformState.pInputs->InputProcessMouseMove(mouseX, mouseY);
                break;
            } 
            case WM_MOUSEWHEEL: 
            {
                int32_t zDelta = GET_WHEEL_DELTA_WPARAM(w_param);
                if (zDelta != 0) 
                {
                    // Flatten the input to an OS-independent (-1, 1)
                    zDelta = (zDelta < 0) ? -1 : 1;
                    inl_pPlatformState.pInputs->InputProcessMouseWheel(zDelta);
                }
                break;
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
                if (button != BlitzenCore::MouseButton::MaxButtons)
                {
                    inl_pPlatformState.pInputs->InputProcessButton(button, bPressed);
                }
                break;
            } 
        }
        return DefWindowProcA(winWindow, msg, w_param, l_param); 
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
}
#endif