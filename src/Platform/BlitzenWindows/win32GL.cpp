#if defined(_WIN32)

#include "Platform/blitPlatformContext.h"
#include "Renderer/BlitzenGL/openglRenderer.h"
#include <GL/wglew.h>

namespace BlitzenPlatform
{
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
        if (!formatIndex)
        {
            BLIT_ERROR("Failed to choose pixel format");
            return 0;
        }
        if (!SetPixelFormat(hdc, formatIndex, &pfd))
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
        if (!wglMakeCurrent(hdc, platform->m_hglrc))
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
        int const createAttribs[] = { WGL_CONTEXT_MAJOR_VERSION_ARB, 4, WGL_CONTEXT_MINOR_VERSION_ARB,  6,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0 };
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
}

#endif