#if defined(_WIN32)
#include "dx12Init.h"
#include "Renderer/Interface/blitRenderer.h"
#include "Renderer/BlitzenDX12/RuntimeHelpers/dx12Commands.h"
#include "Renderer/BlitzenDX12/Resources/dx12RNDResources.h"
#include "Renderer/BlitzenDX12/Resources/dx12Pipelines.h"
#include "Core/DbLog/blitLogger.h"

namespace BlitzenEngine
{
    uint8_t StartupRenderer(BlitzenDX12::Dx12Renderer* pRenderer, uint32_t windowWidth, uint32_t windowHeight, BlitzenPlatform::PlatformContext* pPlatform)
    {
#if !defined(BLIT_DOUBLE_BUFFERING)

        BLIT_WARN("Double buffering enabled by default for Dx12");

#endif

        if (!BlitzenDX12::CreateFactory(&pRenderer->m_factory, pRenderer->m_debugController))
        {
            BLIT_ERROR("Failed to create DXGI factory");
            return 0;
        }

        if (!BlitzenDX12::ChooseAdapter(pRenderer->m_factory.Get(), pRenderer->m_chosenAdapter.ReleaseAndGetAddressOf()))
        {
            BLIT_ERROR("Failed to choose adapter");
            return 0;
        }

        if (!BlitzenDX12::CreateDevice(pRenderer->m_chosenAdapter.Get(), pRenderer->m_device.ReleaseAndGetAddressOf()))
        {
            BLIT_ERROR("Failed to create device");
            return 0;
        }

        if (!pRenderer->m_device)
        {
            BLIT_ERROR("Device is null");
            return 0;
        }

        if (!BlitzenDX12::CheckForDeviceRemoval(pRenderer->m_device.Get()))
        {
            BLIT_ERROR("Device removed");
            return 0;
        }

        if constexpr (BlitzenDX12::Ce_GPUValidationRequested)
        {
            if (!BlitzenDX12::CreateDebugController(pRenderer->m_debugController.Get(), pRenderer->m_debugController1, pRenderer->m_device.Get()))
            {
                BLIT_ERROR("Failed to create debug controller 1");
                return 0;
            }
        }
        //TODO: Think about adding ID3D12DebugDevice or whatever it is called to watchout for unfreed resources

        if (!BlitzenDX12::CreateCommandQueue(pRenderer->m_device.Get(), pRenderer->m_commandQueue.ReleaseAndGetAddressOf(), D3D12_COMMAND_QUEUE_FLAG_NONE, D3D12_COMMAND_LIST_TYPE_DIRECT))
        {
            BLIT_ERROR("Failed to create command queue");
            return 0;
        }

        if (!BlitzenDX12::CreateCommandQueue(pRenderer->m_device.Get(), pRenderer->m_transferCommandQueue.ReleaseAndGetAddressOf(), D3D12_COMMAND_QUEUE_FLAG_NONE, D3D12_COMMAND_LIST_TYPE_COPY))
        {
            BLIT_ERROR("Failed to create transfer command queue");
            return 0;
        }

        if (!BlitzenDX12::CreateCommandQueue(pRenderer->m_device.Get(), pRenderer->m_computeCommandQueue.ReleaseAndGetAddressOf(), D3D12_COMMAND_QUEUE_FLAG_NONE, D3D12_COMMAND_LIST_TYPE_COMPUTE))
        {
            BLIT_ERROR("Failed to create compute command queue");
            return 0;
        }

        for (uint32_t i = 0; i < BlitzenDX12::ce_framesInFlight; i++)
        {
            if (!pRenderer->m_cmdContext[i].Init(pRenderer->m_device.Get()))
            {
                BLIT_ERROR("Failed to create frame tools");
                return 0;
            }
        }

        if (!BlitzenDX12::CreateSwapchain(pRenderer->m_factory.Get(), pRenderer->m_commandQueue.Get(), windowWidth, windowHeight, pPlatform->m_hwnd, &pRenderer->m_swapchain))
        {
            BLIT_ERROR("Failed to create swapchain");
            if (BlitzenDX12::CheckForDeviceRemoval(pRenderer->m_device.Get()))
            {
                BLIT_INFO("Device was not removed");
            }
            return 0;
        }
        pRenderer->m_swapchainWidth = windowWidth;
        pRenderer->m_swapchainHeight = windowHeight;

        if (!CreateDescriptorHeaps(pRenderer->m_device.Get(), pRenderer->m_descriptorContext))
        {
            BLIT_ERROR("Failed to create descriptor heaps");
            return 0;
        }

        if (!CreateSwapchainResources(pRenderer->m_swapchain.Get(), pRenderer->m_device.Get(), pRenderer->m_swapchainBackBuffers, pRenderer->m_descriptorContext))
        {
            BLIT_ERROR("Failed to create swapchain back buffers");
            return 0;
        }

        if (!BlitzenDX12::CreateDepthTargets(pRenderer->m_device.Get(), pRenderer->m_depthBuffers, pRenderer->m_descriptorContext, pRenderer->m_swapchainWidth, pRenderer->m_swapchainHeight))
        {
            BLIT_ERROR("Failed to create swapchain back buffers");
            return 0;
        }

        if (!BlitzenDX12::CreateRootSignatures(pRenderer->m_device.Get(), pRenderer->m_pipelineContext, pRenderer->m_descriptorContext))
        {
            BLIT_ERROR("%s: Failed to create root signatures", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return 0;
        }

        if (!BlitzenDX12::CreateCmdSignatures(pRenderer->m_device.Get(), pRenderer->m_pipelineContext))
        {
            BLIT_ERROR("%s: Failed to create command signatures", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return 0;
        }

        BlitzenDX12::CreateOMSTargetDescs(pRenderer->m_pipelineContext.m_renderTargetPassDesc, pRenderer->m_pipelineContext.m_depthTargetPassDesc, pRenderer->m_descriptorContext.m_swapchainRtvHandle, 
            pRenderer->m_descriptorContext.m_depthTargetDSVHandle);

        if (!BlitzenDX12::CreateTriangleGraphicsPipeline(pRenderer->m_device.Get(), pRenderer->m_pipelineContext.m_triangleRoot, pRenderer->m_pipelineContext.m_trianglePso.ReleaseAndGetAddressOf()))
        {
            BLIT_ERROR("Failed to create triangle graphics pipeline");
            return 0;
        }

        if (!BlitzenDX12::CreatePipelines(pRenderer->m_device.Get(), pRenderer->m_pipelineContext))
        {
            BLIT_ERROR("%s: Failed to create pipelines", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return 0;
        }

        // Texture sampler offsets before creation
        pRenderer->m_descriptorContext.m_texSmpOffset = pRenderer->m_descriptorContext.m_samplerHeapCurrentOffset;
        pRenderer->m_descriptorContext.m_texSmpHandle = pRenderer->m_descriptorContext.m_samplerHeapHandle;
        pRenderer->m_descriptorContext.m_texSmpHandle.ptr += pRenderer->m_descriptorContext.m_texSmpOffset * pRenderer->m_descriptorContext.m_samplerHeapIncrement;
        BlitzenDX12::CreateSampler(pRenderer->m_device.Get(), pRenderer->m_descriptorContext, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, nullptr, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

        if (!BlitzenDX12::CreateRWResources(pRenderer->m_device.Get(), pRenderer->m_rwResources, pRenderer->m_swapchainWidth, pRenderer->m_swapchainHeight))
        {
            BLIT_ERROR("%s: Failed to create read write resources", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return 0;
        }

        if (!BlitzenDX12::CreateROResources(pRenderer->m_device.Get(), pRenderer->m_roResources))
        {
            BLIT_ERROR("%s: Failed to create read only resources", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return 0;
        }

        if (!BlitzenDX12::CreateCpuLogicBuffers(pRenderer->m_device.Get(), pRenderer->MCpuLogicBuffers))
        {
            BLIT_ERROR("%s: Failed to create read only resources", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return 0;
        }

        return 1;
    }
}

#endif