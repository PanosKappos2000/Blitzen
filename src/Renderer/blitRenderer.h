#pragma once

// Vulkan
#include "BlitzenVulkan/vulkanRenderer.h"

// Opengl
#include "BlitzenGl/openglRenderer.h"

// Direct3D12
#ifdef _WIN32
#include "BlitzenDX12/dx12Renderer.h"
#endif

namespace BlitzenEngine
{
    #if defined(BLIT_GL_LEGACY_OVERRIDE) && defined(BLITZEN_VULKAN_OVERRIDE) && defined(_WIN32)

        using Renderer = 
        BlitCL::SmartPointer<BlitzenGL::OpenglRenderer, BlitzenCore::AllocationType::Renderer>;

        using RendererPtrType = BlitzenGL::OpenglRenderer*;

		using RendererType = BlitzenGL::OpenglRenderer;

    #elif defined(BLITZEN_VULKAN_OVERRIDE) && defined(_WIN32)

        using Renderer = 
        BlitCL::SmartPointer<BlitzenDX12::Dx12Renderer, BlitzenCore::AllocationType::Renderer>;

        using RendererPtrType = BlitzenDX12::Dx12Renderer*;

		using RendererType = BlitzenDX12::Dx12Renderer;

    #else

        using Renderer = 
        BlitCL::SmartPointer<BlitzenVulkan::VulkanRenderer, BlitzenCore::AllocationType::Renderer>;

        using RendererPtrType = BlitzenVulkan::VulkanRenderer*;

        using RendererType = BlitzenVulkan::VulkanRenderer;

    #endif
}