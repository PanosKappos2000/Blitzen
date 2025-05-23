#if defined(_WIN32)

#include "blitPlatformContext.h"
#include "Common/blitMappedFile.h"
#include <cstring>
#include <windowsx.h>
#include <WinUser.h>
#include "Renderer/BlitzenVulkan/vulkanData.h"
#include <vulkan/vulkan_win32.h>
#include "Renderer/BlitzenGl/openglRenderer.h"
#include "Renderer/Interface/blitRenderer.h"
#include <GL/wglew.h>
#include "blitPlatform.h"
#include "Core/Events/blitEvents.h"
#include "Core/blitzenEngine.h"
#include "Game/blitCamera.h"

namespace BlitzenPlatform
{
    // EVENT CALLBACK
    LRESULT CALLBACK Win32ProcessMessage(HWND hwnd, uint32_t msg, WPARAM w_param, LPARAM l_param);

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

        return hwnd;
    }

    bool PlatformStartup(const char* appName, void* pPlatform, void* pEvents, void* pRenderer)
    {
		auto platform{ reinterpret_cast<BlitzenPlatform::PlatformContext*>(pPlatform) };

        HINSTANCE hInstance = GetModuleHandleA(nullptr);
        HWND hwnd = CreateStandardWindow(hInstance, BlitzenCore::Ce_InitialWindowWidth, BlitzenCore::Ce_InitialWindowHeight, appName);
        if (!hwnd)
        {
            BLIT_FATAL("Window creation failed");
            return false;
        }

        // UPDATES
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pEvents));
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        
        // SAVE
        platform->m_hinstance = hInstance;
        platform->m_hwnd = hwnd;
        
        // BACKEND RENDERING API INIT
		auto pBackendRenderer = reinterpret_cast<BlitzenEngine::RendererPtrType>(pRenderer);
        if (!pBackendRenderer->Init(BlitzenCore::Ce_InitialWindowWidth, BlitzenCore::Ce_InitialWindowHeight, platform))
        {
            BLIT_FATAL("Failed to initialize rendering API");
            return false;
        }

        // Success
        return true;
    }

    void BlitzenSleep(uint64_t ms)
    {
        Sleep(static_cast<DWORD>(ms));
    }

    /*
        TIME MANAGER
    */
    void PlatfrormSetupClock(BlitzenCore::WorldTimerManager* pClock)
    {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);

        pClock->m_clockFrequency = 1.0 / double(frequency.QuadPart);
        //QueryPerformanceCounter(&inl_startTime); LARGE_INTEGER startTime never used
    }

    double PlatformGetAbsoluteTime(double clockFrequency)
    {
        LARGE_INTEGER nowTime;
        QueryPerformanceCounter(&nowTime);
        return double(nowTime.QuadPart) * clockFrequency;
    }

    /*
        LOGGING
    */
    static void BlitWinLogSetup(const char* message, uint8_t color, HANDLE consoleHandle)
    {
        SetConsoleTextAttribute(consoleHandle, BlitzenCore::CE_PLATFORM_CONSOLE_LOGGER_COLORS[color]);

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

    void PlatformLoggerFileWrite(const char* message, uint8_t color)
    {
        static MEMORY_MAPPED_FILE_SCOPE s_scopedFile;

        if (s_scopedFile.m_pFileView == INVALID_HANDLE_VALUE)
        {
            auto mmfResult{ s_scopedFile.Open("blitLogOutput.txt", FileModes::Write, BlitzenCore::Ce_BlitLogOutputFileSize) };
            if (mmfResult != BLIT_MMF_RES::SUCCESS)
            {
                const char* mmfErrorString{ GET_BLIT_MMF_RES_ERROR_STR(mmfResult) };
                PlatformConsoleError(mmfErrorString, (uint8_t)BlitzenCore::LogLevel::Error);
                PlatformConsoleWrite(message, color);
                return;
            }
        }

        WriteMemoryMappedFile(s_scopedFile, s_scopedFile.m_endOffset, strlen(message), const_cast<char*>(message));
        
    }

    void PlatformLoggerFileError(const char* message, uint8_t color)
    {
        PlatformLoggerFileWrite(message, color);
    }

    /*
        VULKAN
    */
    uint8_t CreateVulkanSurface(VkInstance& instance, VkSurfaceKHR& surface, VkAllocationCallbacks* pAllocator, void* pPlatform)
    {
        auto platform{ reinterpret_cast<BlitzenPlatform::PlatformContext*>(pPlatform) };

        VkWin32SurfaceCreateInfoKHR info = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
        info.hinstance = platform->m_hinstance;
        info.hwnd = platform->m_hwnd;

        auto res = vkCreateWin32SurfaceKHR(instance, &info, pAllocator, &surface);
        if (res != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create Vulkan surface");
            return 0;
        }
        return 1;
    }

    /*
        OPENGL
    */
    uint8_t CreateOpenglDrawContext(void* pPlatform)
    {
        auto platform{ reinterpret_cast<BlitzenPlatform::PlatformContext*>(pPlatform) };

        // Get the device context of the window
        auto hdc = GetDC(platform->m_hwnd);

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
			BLIT_ERROR("Failed to choose pixel format");
            return 0;
        }
        if(!SetPixelFormat(hdc, formatIndex, &pfd))
        {
			BLIT_ERROR("Failed to set pixel format");
            return 0;
        }
        if (!DescribePixelFormat(hdc, formatIndex, sizeof(pfd), &pfd))
        {
			BLIT_ERROR("Failed to describe pixel format");
            return 0;
        }

        if ((pfd.dwFlags & PFD_SUPPORT_OPENGL) != PFD_SUPPORT_OPENGL)
        {
			BLIT_ERROR("Pixel format does not support OpenGL");
            return 0;
        }

        // Create the dummy render context
        platform->m_hglrc = wglCreateContext(hdc);

        // Make this the current context so that glew can be initialized
        if(!wglMakeCurrent(hdc, platform->m_hglrc))
        {
			BLIT_ERROR("Failed to make OpenGL context current");
            return 0;
        }

        // Initializes glew
        if (glewInit() != GLEW_OK)
        {
			BLIT_ERROR("Failed to initialize GLEW");
            return 0;
        }

        // With glew now available, extension function pointers can be accessed and a better gl context can be retrieved
        // So the old render context is deleted and the device is released
        wglDeleteContext(platform->m_hglrc);
        ReleaseDC(platform->m_hwnd, hdc);

        // Get a new device context
        hdc = GetDC(platform->m_hwnd);

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
        platform->m_hglrc = wglCreateContextAttribsARB(hdc, 0, createAttribs);

        // Set the gl render context as the new one
        return (wglMakeCurrent(hdc, platform->m_hglrc));
    }

    void OpenglSwapBuffers(BlitzenPlatform::PlatformContext* pPlatform)
    {
        #if defined(BLIT_VSYNC)
            wglSwapIntervalEXT(1);
        #else
            wglSwapIntervalEXT(0);
        #endif

        wglSwapLayerBuffers(GetDC(pPlatform->m_hwnd), WGL_SWAP_MAIN_PLANE);
    }

    /*
        EVENT SYSTEM
    */
    bool DispatchEvents(void* pPlatform)
    {
        MSG message;

        while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        return true;
    }

    // CALLBACK
    LRESULT CALLBACK Win32ProcessMessage(HWND hwnd, uint32_t msg, WPARAM w_param, LPARAM l_param)
    {
        auto pEventSystem = reinterpret_cast<BlitzenCore::EventSystem*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

        switch(msg)
        {
            case WM_ERASEBKGND:
            {
                // Notify the OS that erasing will be handled by the application to prevent flicker
                return 1;
            }
            case WM_CLOSE:
            {
                return pEventSystem->FireEvent(BlitzenCore::BlitEventType::EngineShutdown);
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
                GetClientRect(hwnd, &rect);

                uint32_t width = rect.right - rect.left;
                uint32_t height = rect.bottom - rect.top;

                auto& camera{ pEventSystem->m_blitzenContext.pCameraContainer->GetMainCamera()};

                auto oldWidth = camera.transformData.windowWidth;
                auto oldHeight = camera.transformData.windowHeight;

                camera.transformData.windowWidth = float(width);
                camera.transformData.windowHeight = float(height);

                if (!pEventSystem->FireEvent(BlitzenCore::BlitEventType::WindowUpdate))
                {
                    camera.transformData.windowWidth = oldWidth;
                    camera.transformData.windowHeight = oldHeight;
                }
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

                pEventSystem->InputProcessKey(key, bPressed);

                break;
            } 

            case WM_MOUSEMOVE: 
            {
                
                int32_t mouseX = GET_X_LPARAM(l_param);
                int32_t mouseY = GET_Y_LPARAM(l_param);

                pEventSystem->InputProcessMouseMove(mouseX, mouseY);

                break;
            } 
            case WM_MOUSEWHEEL: 
            {
                int32_t zDelta = GET_WHEEL_DELTA_WPARAM(w_param);
                if (zDelta != 0) 
                {
                    // Flatten the input to an OS-independent (-1, 1)
                    zDelta = (zDelta < 0) ? -1 : 1;

                    pEventSystem->InputProcessMouseWheel(zDelta);
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
                    pEventSystem->InputProcessButton(button, bPressed);
                }
                break;
            } 
        }
        return DefWindowProcA(hwnd, msg, w_param, l_param); 
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

    static void PlatformShutdown(PlatformContext* P_HANDLE)
    {
        if (P_HANDLE->m_hglrc)
        {
            wglDeleteContext(P_HANDLE->m_hglrc);
        }

        if (P_HANDLE->m_hwnd)
        {
            DestroyWindow(P_HANDLE->m_hwnd);
        }
    }

    PlatformContext::~PlatformContext()
    {
		PlatformShutdown(this);
    }
}
#endif