#if defined(_WIN32)

#include "Platform/blitPlatformContext.h"
#include <cstring>
#include <windowsx.h>
#include <WinUser.h>
#include "Platform/blitPlatform.h"
#include "Renderer/View/blitCamera.h"
#include "Renderer/Interface/blitRenderer.h"
#include "Core/Events/blitEvents.h"
#include "backends/imgui_impl_win32.h"
#include "Core/Dasher/Interface/dasherInterface.h"

#if defined(DASHER_JOIN) && defined(DASHER_USE_DEAR)
    
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif

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

    static bool DasherPlatformInit(BlitzenIMGUI::DasherEditor* pDasher, HWND hwnd)
    {
        if (!ImGui_ImplWin32_Init(hwnd))
        {
            BLIT_ERROR("Failed to initialize imgui win32 impl");
            return 0;
        }
        return true;
    }

    bool SystemStartup(PlatformArgs& args)
    {
		auto pPlatform{ reinterpret_cast<BlitzenPlatform::PlatformContext*>(args.m_pPlatform) };

        HINSTANCE hInstance = GetModuleHandleA(nullptr);
        HWND hwnd = CreateStandardWindow(hInstance, BlitzenCore::Ce_InitialWindowWidth, BlitzenCore::Ce_InitialWindowHeight, BlitzenCore::CE_BLITZEN);
        if (!hwnd)
        {
            BLIT_FATAL("Window creation failed");
            return false;
        }

        // UPDATES
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(args.m_pEvents));
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        
        // SAVE
        pPlatform->m_hinstance = hInstance;
        pPlatform->m_hwnd = hwnd;
        
        // BACKEND RENDERING API INIT
		auto pRenderer = reinterpret_cast<BlitzenEngine::RendererPtrType>(args.m_pRenderer);
        if (!pRenderer->Init(BlitzenCore::Ce_InitialWindowWidth, BlitzenCore::Ce_InitialWindowHeight, pPlatform))
        {
            BLIT_FATAL("Failed to initialize rendering API");
            return false;
        }

#if defined(DASHER_JOIN)

        auto pDasher = reinterpret_cast<BlitzenCore::Dasher*>(args.m_pEditor);

        if (!pDasher->Init(pRenderer))
        {
            BLIT_FATAL("Failed to initialize Dasher Editor");
            return false;
        }

        if (!DasherPlatformInit(pDasher, hwnd))
        {
            BLIT_FATAL("Failed to initialize Dasher Editor");
            return false;
        }

        pDasher->SetWindow(BlitzenCore::Ce_InitialWindowWidth, BlitzenCore::Ce_InitialWindowHeight);

#endif

        // Success
        return true;
    }

    void BlitzenSleep(uint64_t ms)
    {
        Sleep(static_cast<DWORD>(ms));
    }

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
    LRESULT CALLBACK Win32ProcessMessage(HWND hwnd, uint32_t msg, WPARAM wparam, LPARAM lparam)
    {
        auto pEventSystem = reinterpret_cast<BlitzenCore::EventSystem*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

#if defined(DASHER_JOIN) && defined(DASHER_USE_DEAR)

        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);

#endif

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

                auto key = BlitzenCore::BlitKey(wparam);

                pEventSystem->InputProcessKey(key, bPressed);

                break;
            } 

            case WM_MOUSEMOVE: 
            {
                
                int32_t mouseX = GET_X_LPARAM(lparam);
                int32_t mouseY = GET_Y_LPARAM(lparam);

                pEventSystem->InputProcessMouseMove(mouseX, mouseY);

                break;
            } 
            case WM_MOUSEWHEEL: 
            {
                int32_t zDelta = GET_WHEEL_DELTA_WPARAM(wparam);
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
        return DefWindowProcA(hwnd, msg, wparam, lparam); 
    }

    void DearDasherUpdate()
    {
        ImGui_ImplWin32_NewFrame();
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