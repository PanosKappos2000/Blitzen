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
#include <direct.h>
#include <xstring>

// Needs to make sure that IMGUI callback does not get overriden
#if defined(DASHER_JOIN) && defined(DASHER_USE_DEAR)
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

namespace BlitzenPlatform
{
    constexpr SHORT CE_HID_USAGE_PAGE_GENERIC = 0x01;
    constexpr SHORT CE_HID_USAGE_MOUSE = 0x02;
    constexpr SHORT CE_HID_USAGE_KEYBOARD = 0x06;
    constexpr SHORT CE_HID_USAGE_JOYSTICK = 0x04;

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
        
        reinterpret_cast<BlitzenWorld::BLITZEN_SYSTEM_CONTEXT*>(args.SYSTEM)->pPlatform = pPlatform;

        // UPDATES
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(args.SYSTEM));
        //ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        
        // SAVE
        pPlatform->m_hinstance = hInstance;
        pPlatform->m_hwnd = hwnd;

        // RAW INPUT
        RAWINPUTDEVICE rid[2];
        rid[0].usUsagePage = CE_HID_USAGE_PAGE_GENERIC;  
        rid[0].usUsage = CE_HID_USAGE_MOUSE;      
        rid[0].dwFlags = 0;
        rid[0].hwndTarget = hwnd; 
		rid[1].usUsagePage = CE_HID_USAGE_PAGE_GENERIC;
		rid[1].usUsage = CE_HID_USAGE_KEYBOARD;
		rid[1].dwFlags = 0;
		rid[1].hwndTarget = hwnd;
        if (!RegisterRawInputDevices(rid, BLIT_ARRAY_SIZE(rid), sizeof(RAWINPUTDEVICE)))
        {
            MessageBox(hwnd, "Failed to register raw input device", "Error", MB_OK);
            return false;
        }

        pPlatform->SCOPED_RAW_INPUT_BYTE.ALLOC(BLITWIN32_SCOPED_RAWINPUT_BYTE_FIXED_SIZE);
        
        // BACKEND RENDERING API INIT
        auto pRenderer{ reinterpret_cast<BlitzenEngine::RendererPtrType>(args.m_pRenderer) };
        if (!BlitzenEngine::StartupRenderer(pRenderer, BlitzenCore::Ce_InitialWindowWidth, BlitzenCore::Ce_InitialWindowHeight, pPlatform))
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

        auto SYSTEM{ reinterpret_cast<BlitzenWorld::BLITZEN_SYSTEM_CONTEXT*>(args.SYSTEM) };

        BlitzenCore::RegisterDefaultEvents(SYSTEM);

#if defined(DASHER_JOIN) && defined(DASHER_USE_DEAR)

        BlitzenCore::AssignEditorCallbacks(SYSTEM);

#endif

        BlitzenWorld::BlitzenSystemsInit(SYSTEM);

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

    void MakeWindowVisible(BLIT_STRAIGHTHANDLE handle)
    {
        PlatformContext* ptrBP_HANDLE = reinterpret_cast<PlatformContext*>(handle);

        ShowWindow(ptrBP_HANDLE->m_hwnd, SW_SHOW);
        UpdateWindow(ptrBP_HANDLE->m_hwnd);
    }

    void PutMouseInGameState(PlatformContext* ptrBP_HANDLE)
    {
        while(ShowCursor(FALSE) >= 0);
        UpdateWindow(ptrBP_HANDLE->m_hwnd);

        // Optionally lock the cursor within the window bounds
        //ClipCursor(ptrHandle->);
    }

    void GetSystemMousePos(PlatformContext* ptrBP_HANDLE, int16_t& mouseXData, int16_t mouseYData)
    {
        POINT mousePos;
        GetCursorPos(&mousePos);  
        ScreenToClient(ptrBP_HANDLE->m_hwnd, &mousePos);

        mouseXData = (int16_t)mousePos.x;
        mouseYData = (int16_t)mousePos.y;
    }

    void BLITWIN32_SCOPED_RAWINPUT_BYTE::ALLOC(UINT dwSize)
    {
        m_lpb = reinterpret_cast<BYTE*>(PlatformMalloc(dwSize, 0));
    }

    BLITWIN32_SCOPED_RAWINPUT_BYTE::~BLITWIN32_SCOPED_RAWINPUT_BYTE()
    {
        PlatformFree(m_lpb, false);
    }

    // CALLBACK
    LRESULT CALLBACK Win32ProcessMessage(HWND hwnd, uint32_t msg, WPARAM wparam, LPARAM lparam)
    {
        auto SYSTEM = reinterpret_cast<BlitzenWorld::BLITZEN_SYSTEM_CONTEXT*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

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
                return BlitzenCore::DispatchEventCallback(SYSTEM, BlitzenCore::BlitEventType::EngineShutdown);
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

                uint32_t oldWidth = SYSTEM->WIDTH;
                uint32_t oldHeight = SYSTEM->HEIGHT;

                SYSTEM->WIDTH = width;
                SYSTEM->HEIGHT = height;

                if (!BlitzenCore::DispatchEventCallback(SYSTEM, BlitzenCore::BlitEventType::WindowUpdate))
                {
                    SYSTEM->WIDTH = oldWidth;
                    SYSTEM->HEIGHT = oldHeight;
                }
                break;
            }

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
                break;

            case WM_INPUT:
            {
                //BLIT_INFO("RAW INPUT");
                UINT dwSize{ 0 };
                GetRawInputData((HRAWINPUT)lparam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));

                if (dwSize >= BLITWIN32_SCOPED_RAWINPUT_BYTE_FIXED_SIZE)
                {
                    break;
                }

                auto p_BP_HANDLE = reinterpret_cast<PlatformContext*>(SYSTEM->pPlatform);

                if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, p_BP_HANDLE->SCOPED_RAW_INPUT_BYTE.m_lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
                {
                    break;
                }

                RAWINPUT* raw = (RAWINPUT*)p_BP_HANDLE->SCOPED_RAW_INPUT_BYTE.m_lpb;

                switch (raw->header.dwType)
                {
                case RIM_TYPEMOUSE:
                {
                    // Extract relative movement
                    SYSTEM->mMouseDeltaXAxis += raw->data.mouse.lLastX;  
                    SYSTEM->mMouseDeltaYAxis += raw->data.mouse.lLastY;

                    break;
                }
                case RIM_TYPEKEYBOARD:
                {
                    // Extract key information
                    auto& keyboard = raw->data.keyboard;
                    auto key = BlitzenCore::BlitKey(keyboard.VKey);
                    BlitzenCore::InputProcessKey(SYSTEM, key, !(keyboard.Flags & RI_KEY_BREAK));
                    break;
				}
                default:
                {
                    break;
                }
                }

                break;
            }
            case WM_MOUSEWHEEL: 
            {
                int32_t zDelta = GET_WHEEL_DELTA_WPARAM(wparam);
                if (zDelta != 0) 
                {
                    // Flatten the input to an OS-independent (-1, 1)
                    zDelta = (zDelta < 0) ? -1 : 1;

                    BlitzenCore::InputProcessMouseWheel(SYSTEM, zDelta);
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
                    BlitzenCore::InputProcessButton(SYSTEM, button, bPressed);
                }

                break;
            }
            default:
                break;
        }
        return DefWindowProcA(hwnd, msg, wparam, lparam); 
    }

    void LogWin32HresultError(HRESULT res)
    {
        _com_error err{ res };
        BLIT_ERROR("WIN32.HRESULT.ERROR_MESSAGE: %s", err.ErrorMessage());
    }

    void DearDasherUpdate()
    {
        ImGui_ImplWin32_NewFrame();
    }

    bool CreateDirectoryIfMissing(const char* path)
    {
        return _mkdir(path) == 0 || errno == EEXIST;
    }

    static std::wstring WideFromUTF8(const char* s)
    {
        if (!s) return {};

        int32_t wlen = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
        if (wlen <= 0) return {};

        std::wstring w;
        w.resize(size_t(wlen - 1));
        MultiByteToWideChar(CP_UTF8, 0, s, -1, w.data(), wlen);

        return w;
    }

    bool PlatformCopyFile(const char* source, const char* dest, bool overwriteFlag)
    {
        std::wstring wsrc = WideFromUTF8(source);
        std::wstring wdst = WideFromUTF8(dest);
        if (wsrc.empty() || wdst.empty()) return false;

        // CopyFileW third param: bFailIfExists (TRUE = do not overwrite)
        const BOOL ok = CopyFileW(wsrc.c_str(), wdst.c_str(), overwriteFlag ? FALSE : TRUE);
        return ok != 0;
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