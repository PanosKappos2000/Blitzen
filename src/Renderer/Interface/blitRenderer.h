#pragma once
#include "blitRendererInterface.h"
#include "BlitzenVulkan/vulkanRenderer.h"
#if defined(_WIN32)
    #include "BlitzenGl/openglRenderer.h"
#endif
#if defined(_WIN32)
    #include "BlitzenDX12/dx12Renderer.h"
#endif
#include <typeinfo>
#include <cstring>

namespace BlitzenEngine
{
    #if defined(linux)
        using Renderer = BlitCL::SmartPointer<BlitzenVulkan::VulkanRenderer, BlitzenCore::AllocationType::Renderer>;

        using RendererPtrType = BlitzenVulkan::VulkanRenderer*;

        using RendererType = BlitzenVulkan::VulkanRenderer;

    #elif defined(_WIN32) && defined(BLIT_VK_FORCE)

        using Renderer = BlitCL::SmartPointer<BlitzenVulkan::VulkanRenderer, BlitzenCore::AllocationType::Renderer>;

        using RendererPtrType = BlitzenVulkan::VulkanRenderer*;

        using RendererType = BlitzenVulkan::VulkanRenderer;

    #elif defined(_WIN32) && defined(BLIT_GL_LEGACY_OVERRIDE) 

        using Renderer = BlitCL::SmartPointer<BlitzenGL::OpenglRenderer, BlitzenCore::AllocationType::Renderer>;

        using RendererPtrType = BlitzenGL::OpenglRenderer*;

		using RendererType = BlitzenGL::OpenglRenderer;

    #elif defined(_WIN32)

        using Renderer = BlitCL::SmartPointer<BlitzenDX12::Dx12Renderer, BlitzenCore::AllocationType::Renderer>;

        using RendererPtrType = BlitzenDX12::Dx12Renderer*;

        using RendererType = BlitzenDX12::Dx12Renderer;

    #else

        static_assert(true);

    #endif

    #if defined(_WIN32)
        void UpdateRendererTransform(BlitzenDX12::Dx12Renderer* pDx12, RendererTransformUpdateContext& context);
        void UpdateRendererTransform(BlitzenGL::OpenglRenderer* pGL, RendererTransformUpdateContext& context);
    #endif
		void UpdateRendererTransform(BlitzenVulkan::VulkanRenderer* pVK, RendererTransformUpdateContext& context);
    
}