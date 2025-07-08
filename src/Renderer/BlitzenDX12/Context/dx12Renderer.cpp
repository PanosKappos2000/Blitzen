#if defined(_WIN32)
#include "dx12Renderer.h"
#include "Renderer/BlitzenDX12/Resources/dx12Resources.h"
#include "Core/DbLog/blitLogger.h"

namespace BlitzenDX12
{
    Dx12Renderer::~Dx12Renderer()
    {
        auto& cmdContext = m_cmdContext[m_currentFrame];

        PlaceFence(cmdContext.m_frameFence.m_value, m_commandQueue.Get(), cmdContext.m_frameFence.m_dx12Handle.Get(), cmdContext.m_frameFence.m_event);
        PlaceFence(cmdContext.m_computeFence.m_value, m_computeCommandQueue.Get(), cmdContext.m_computeFence.m_dx12Handle.Get(), cmdContext.m_computeFence.m_event);
        PlaceFence(cmdContext.m_copyFence.m_value, m_transferCommandQueue.Get(), cmdContext.m_copyFence.m_dx12Handle.Get(), cmdContext.m_copyFence.m_event);
    }

    uint8_t CheckForDeviceRemoval(ID3D12Device* device)
    {
        HRESULT removalReason = device->GetDeviceRemovedReason();

        if (FAILED(removalReason))
        {
            _com_error err{ removalReason };
            BLIT_FATAL("Device removal reason: %s", err.ErrorMessage());
            return 0;
        }

        // Safe
        return 1;
    }

    uint8_t LOG_ERROR_MESSAGE_AND_RETURN(HRESULT res)
    {
        _com_error err{ res };
        BLIT_ERROR("Dx12Error: %s", err.ErrorMessage());
        return 0;
    }
}

#endif