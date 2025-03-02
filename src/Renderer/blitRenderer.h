#pragma once

#include "BlitzenVulkan/vulkanRenderer.h"
#include "BlitzenGl/openglRenderer.h"
#ifdef _WIN32
#include "BlitzenDX12/dx12Renderer.h"
#endif
#include "Renderer/blitRenderingResources.h"
#include "Game/blitCamera.h"

namespace BlitzenEngine
{
    class RenderingSystem
    {
    public:
        virtual uint8_t Init(uint32_t windowWidth, uint32_t windowHeight) = 0;

        virtual uint8_t UploadTexture(BlitzenEngine::DDS_HEADER& header, 
        BlitzenEngine::DDS_HEADER_DXT10& header10, void* pData, const char* filepath) = 0;

        virtual uint8_t SetupForRendering() = 0;

        virtual void DrawFrame(BlitzenEngine::DrawContext context) = 0;

        virtual void Shutdown() = 0;
    };
}