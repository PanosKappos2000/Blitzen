#pragma once
#include "Core/blitLogger.h"
#define GLEW_STATIC
#include <GL/glew.h>

#include "BlitCL/blitzenContainerLibrary.h"
#include "Renderer/blitDDSTextures.h"
#include "Renderer/blitRenderingResources.h"

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
}