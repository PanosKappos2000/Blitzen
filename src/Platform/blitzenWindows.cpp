#if defined(_WIN32)
#include <cstring>
#include <windows.h>
#include <windowsx.h>
#include <WinUser.h>
#include "BlitzenVulkan/vulkanData.h"
#include <vulkan/vulkan_win32.h>
#include "BlitzenGl/openglRenderer.h"
#include "Renderer//blitRenderer.h"
#include <GL/wglew.h>
#include "platform.h"
#include "Core/blitInput.h"
#include "Engine/blitzenEngine.h"
#include "Game/blitCamera.h"

namespace BlitzenPlatform
{

    struct PlatformState
    {
        HINSTANCE hinstance;
        HWND hwnd;

        // gl render context
        HGLRC hglrc;

        BlitzenCore::EventSystemState* pEvents;
        BlitzenCore::InputSystemState* pInputs;
    };

    inline PlatformState inl_pPlatformState;

    void* GetWindowHandle() { return inl_pPlatformState.hwnd; }

    inline double inl_clockFrequency;
    inline LARGE_INTEGER inl_startTime;

    // WINDOW CALLBACK
    LRESULT CALLBACK Win32ProcessMessage(HWND hwnd, uint32_t msg, WPARAM w_param, LPARAM l_param);

    size_t GetPlatformMemoryRequirements()
    {
        return sizeof(PlatformState);
    }

    static HWND CreateStandardWindow(HINSTANCE hInstance, LONG width, LONG height, const char* appName)
    {
        const char* className = "BlitzenStandardWindowClass";

        // Register the window class
        WNDCLASSEXA wc = {};
        wc.cbSize = sizeof(WNDCLASSEXA);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = Win32ProcessMessage;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = className;

        if (!RegisterClassExA(&wc))
        {
            MessageBoxA(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
            return nullptr;
        }

        // Adjust the window size so the client area is correct
        RECT rect = { 0, 0, width, height };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
        LONG winWidth = rect.right - rect.left;
        LONG winHeight = rect.bottom - rect.top;

        // TODO: Maybe add styles?
        HWND hwnd = CreateWindowExA(0,className, appName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, winWidth, winHeight, nullptr, nullptr, hInstance, nullptr);

        if (!hwnd)
        {
            MessageBoxA(NULL, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
            return nullptr;
        }

        // Success
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        return hwnd;
    }

    bool PlatformStartup(const char* appName, BlitzenCore::EventSystemState* pEvents, BlitzenCore::InputSystemState* pInputs, void* pRenderer)
    {
        inl_pPlatformState.pEvents = pEvents;
        inl_pPlatformState.pInputs = pInputs;

        HINSTANCE hInstance = GetModuleHandleA(nullptr);
        HWND hwnd = CreateStandardWindow(hInstance, BlitzenEngine::ce_initialWindowWidth, BlitzenEngine::ce_initialWindowHeight, appName);
        if (!hwnd)
        {
            BLIT_FATAL("Window creation failed");
            return false;
        }

        // These two need to be set before the renderer for some APIs
        inl_pPlatformState.hinstance = hInstance;
        inl_pPlatformState.hwnd = hwnd;
        
		auto pBackendRenderer = reinterpret_cast<BlitzenEngine::RendererPtrType>(pRenderer);
        if (!pBackendRenderer->Init(BlitzenEngine::ce_initialWindowWidth, BlitzenEngine::ce_initialWindowHeight, hwnd))
        {
            BLIT_FATAL("Failed to initialize rendering API");
            return false;
        }

        // Success
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

        if (inl_pPlatformState.hwnd)
        {
            DestroyWindow(inl_pPlatformState.hwnd);
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

    static void BlitWinLogSetup(const char* message, uint8_t color, HANDLE consoleHandle)
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
        BlitWinLogSetup(message, color, consoleHandle);
    }

    void PlatformConsoleError(const char* message, uint8_t color)
    {
        auto consoleHandle = GetStdHandle(STD_ERROR_HANDLE);
        BlitWinLogSetup(message, color, consoleHandle);
    }

    void PlatformSleep(uint64_t ms)
    {
        Sleep(static_cast<DWORD>(ms));
    }

    uint8_t CreateVulkanSurface(VkInstance& instance, VkSurfaceKHR& surface, VkAllocationCallbacks* pAllocator)
    {
        VkWin32SurfaceCreateInfoKHR info = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
        info.hinstance = inl_pPlatformState.hinstance;
        info.hwnd = inl_pPlatformState.hwnd;

        auto res = vkCreateWin32SurfaceKHR(instance, &info, pAllocator, &surface);
        if (res != VK_SUCCESS)
        {
            return 0;
        }
        return 1;
    }

    uint8_t CreateOpenglDrawContext()
    {
        // Get the device context of the window
        auto hdc = GetDC(inl_pPlatformState.hwnd);

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
        ReleaseDC(inl_pPlatformState.hwnd, hdc);

        // Get a new device context
        hdc = GetDC(inl_pPlatformState.hwnd);

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
        #if defined(BLIT_VSYNC)
            wglSwapIntervalEXT(1);
        #else
            wglSwapIntervalEXT(0);
        #endif

        wglSwapLayerBuffers(GetDC(inl_pPlatformState.hwnd), WGL_SWAP_MAIN_PLANE);
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
                inl_pPlatformState.pEvents->FireEvent(BlitzenCore::BlitEventType::EngineShutdown);
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

                auto& camera{ BlitzenEngine::BlitzenWorld_GetMainCamera(inl_pPlatformState.pEvents->ppContext) };
                camera.transformData.windowWidth = float(width);
                camera.transformData.windowHeight = float(height);

			    inl_pPlatformState.pEvents->FireEvent(BlitzenCore::BlitEventType::WindowResize);
                break;
            }

            
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYUP: 
            {
                // press or release
                bool bPressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);

                auto key = BlitzenCore::BlitKey(w_param);

                inl_pPlatformState.pInputs->InputProcessKey(key, bPressed);

                break;
            } 

            case WM_MOUSEMOVE: 
            {
                
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
                uint8_t bPressed = msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN;

                auto button = BlitzenCore::MouseButton::MaxButtons;

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