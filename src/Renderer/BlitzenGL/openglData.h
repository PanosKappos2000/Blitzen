#pragma once

#if defined(_WIN32)
#define GLEW_STATIC
#include <GL/glew.h>
#include "Core/blitzenEngine.h"

namespace BlitzenGL
{
    struct ShaderProgram
    {
        uint32_t handle = GL_NONE;

        ~ShaderProgram();
    };

    struct GlShader
    {
        uint32_t handle = GL_NONE;

        ~GlShader();
    };

    struct GlBuffer
    {
        uint32_t handle = GL_NONE;

        ~GlBuffer();
    };

    struct VertexArray
    {
        uint32_t handle = GL_NONE;

        ~VertexArray();
    };

    struct GlTexture
    {
        uint32_t handle = GL_NONE;

        ~GlTexture();
    };

    using GraphicsProgram = ShaderProgram;
    using ComputeProgram = ShaderProgram;

    struct DrawCmd
    {
        uint32_t  indexCount;
        uint32_t  instanceCount;
        uint32_t  firstIndex;
        uint32_t  vertexOffset;
        uint32_t  firstInstance;
    };
}

#endif