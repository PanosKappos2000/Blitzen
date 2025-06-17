#if defined(_WIN32)
#include "dx12ResourcesUpload.h"
#include "Renderer/Interface/blitRenderer.h"
#include "Core/DbLog/blitLogger.h"

namespace BlitzenEngine
{
	uint8_t UploadResourcesToGPU(BlitzenDX12::Dx12Renderer* pRenderer, DrawContext& drawContext)
	{
		if (!BlitzenEngine::GenerateHlslVertices(drawContext.m_meshes))
		{
			BLIT_ERROR("%s: Failed to generate HLSL vertices", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			if (!BlitzenEngine::GenerateHLSLClusters(drawContext.m_meshes))
			{
				BLIT_ERROR("%s: Failed to generate HLSL clusters", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}
		}

		if (!BlitzenDX12::UploadResourcesToBuffers(pRenderer->m_device.Get(), drawContext, pRenderer->m_roResources, pRenderer->m_rwResources, pRenderer->m_cmdContext[0], 
			pRenderer->m_transferCommandQueue.Get()))
		{
			BLIT_ERROR("%s: Failed to upload resources to GPU buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		BlitzenDX12::CreateResourceViews(pRenderer->m_device.Get(), pRenderer->m_descriptorContext, pRenderer->m_cmdContext[pRenderer->m_currentFrame], pRenderer->m_transferCommandQueue.Get(), 
			pRenderer->m_roResources, pRenderer->m_rwResources, drawContext, pRenderer->m_depthBuffers, pRenderer->m_swapchainWidth, pRenderer->m_swapchainHeight);

		if (!BlitzenDX12::CheckForDeviceRemoval(pRenderer->m_device.Get()))
		{
			BLIT_ERROR("%s: Device removed, possibly after trying to create resources view", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		// Gives the pyramid size to th camera. There is not much reason for the camera to have it, but it is what it is.
		drawContext.m_camera.viewData.pyramidWidth = float(pRenderer->m_rwResources[0].m_HI_Z.width);
		drawContext.m_camera.viewData.pyramidHeight = float(pRenderer->m_rwResources[0].m_HI_Z.height);

		return 1;
	}
}

#endif