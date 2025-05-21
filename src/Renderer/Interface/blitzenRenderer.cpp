#include "Renderer/blitRenderer.h"

namespace BlitzenEngine
{
#if defined(_WIN32)
    void UpdateRendererTransform(BlitzenDX12::Dx12Renderer* pDx12, RendererTransformUpdateContext& context)
    {
		pDx12->UpdateObjectTransform(context);
    }

    void UpdateRendererTransform(BlitzenGL::OpenglRenderer* pGL, RendererTransformUpdateContext& context)
    {
		pGL->UpdateObjectTransform(context);
    }
#endif
    void UpdateRendererTransform(BlitzenVulkan::VulkanRenderer* pVK, RendererTransformUpdateContext& context)
    {
		pVK->UpdateObjectTransform(context);
    }
}