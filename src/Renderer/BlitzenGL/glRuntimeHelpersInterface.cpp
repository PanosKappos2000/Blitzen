#if defined(_WIN32)
#pragma once
#include "Renderer/Interface/blitRenderer.h"

namespace BlitzenEngine
{
    void UpdateRendererWindowData(BlitzenGL::OpenglRenderer* pRenderer, uint32_t newWidth, uint32_t newHeight)
    {
        glViewport(0, 0, GLsizei(newWidth), GLsizei(newHeight));
    }
}

#endif