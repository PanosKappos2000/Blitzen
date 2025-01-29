#pragma once

#define GLEW_STATIC
#include <GL/glew.h>

#include "Core/blitLogger.h"
#include "Core/blitzenContainerLibrary.h"

namespace BlitzenGL
{
    class OpenglRenderer
    {
    public:
        uint8_t Init();
    };
}

namespace BlitzenPlatform
{
    uint8_t CreateOpenglContext();

    uint8_t GetOpenglExtension(GLenum name);
}