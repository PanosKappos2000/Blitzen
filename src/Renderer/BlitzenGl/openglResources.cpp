#include "openglData.h"

namespace BlitzenGL
{
    ShaderProgram::~ShaderProgram()
    {
        if(handle != GL_NONE)
        {
            glDeleteProgram(handle);
        }
    }

    GlShader::~GlShader()
    {
        if(handle != GL_NONE)
            glDeleteShader(handle);
    }

    GlBuffer::~GlBuffer()
    {
        if(handle != GL_NONE)
            glDeleteBuffers(1, &handle);
    }

    VertexArray::~VertexArray()
    {
        if(handle != GL_NONE)
            glDeleteVertexArrays(1, &handle);
    }

    GlTexture::~GlTexture()
    {
        if(handle != GL_NONE)
            glDeleteTextures(1, &handle);
    }
}