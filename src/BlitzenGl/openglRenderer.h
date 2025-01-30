// Temporarily opengl is only available on windows
#if _MSC_VER
#pragma once

#include "Core/blitLogger.h"

#define GLEW_STATIC
#include <GL/glew.h>

#include "Core/blitzenContainerLibrary.h"

#include "Renderer/blitRenderingResources.h"

namespace BlitzenGL
{
    using GraphicsProgram = unsigned int;
    using GlBuffer = unsigned int;
    using GlShader = unsigned int;

    class OpenglRenderer
    {
    public:
        uint8_t Init(uint32_t windowWidth, uint32_t windowHeight);

        void DrawFrame();

        uint8_t SetupForRendering(BlitzenEngine::RenderingResources* pResources);

        void Shutdown();

    private:
        GraphicsProgram m_opaqueGeometryGraphicsProgram;

        GlBuffer m_vertexBuffer;
        GlBuffer m_indexBuffer;
    };

    uint8_t CompileShader(GlShader& shader, GLenum shaderType, const char* filepath);

    uint8_t CreateGraphicsProgram(const char* verteShaderFilepath, const char* fragmentShaderFilepath, 
    GraphicsProgram& program);
}

namespace BlitzenPlatform
{
    uint8_t CreateOpenglDrawContext();

    void OpenglSwapBuffers();
}
#endif