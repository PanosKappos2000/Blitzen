#include "openglRenderer.h"

namespace BlitzenGL
{
    uint8_t OpenglRenderer::Init()
    {
        if(!BlitzenPlatform::CreateOpenglContext())
        {
            BLIT_ERROR("Failed to create opengl context")
            return 0;
        }
        GLenum res = glewInit();
        if (GLEW_OK != res)
        {
            BLIT_ERROR("Opengl failed to load with the glew library")
            return 0;
        }

        return 1;
    }
}