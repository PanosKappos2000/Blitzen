#if defined(_WIN32)

#include "dx12Renderer.h"
#include "Renderer/BlitzenDX12/Resources/dx12Resources.h"

namespace BlitzenDX12
{
    Dx12Renderer::~Dx12Renderer()
    {
        auto& cmdContext = m_cmdContext[m_currentFrame];

        PlaceFence(cmdContext.m_frameFence.m_value, m_commandQueue.Get(), cmdContext.m_frameFence.m_dx12Handle.Get(), cmdContext.m_frameFence.m_event);
    }
}

#endif