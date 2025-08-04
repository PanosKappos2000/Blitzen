#pragma once
#include "Renderer/BlitzenVulkan/Context/vulkanContext.h"
#include "Renderer/Interface/blitRendererInterface.h"

namespace BlitzenVulkan
{
	uint8_t UploadResourcesToBuffers(VkDevice device, VkInstance instance, VmaAllocator vma, VkQueue queue, BlitzenEngine::DrawContext& drawContext,
		ROResources& readOnlies, RWResources* pRWResourcesArray, CommandContext& cmdContext, VulkanStats& stats, LoadingContextMesh& loadingContextMesh, 
		LoadingContextMaterials& loadingContextMaterials);

	void CreateDescriptors(DescriptorContext& descriptorContext, ROResources& roResources, RWResources* rwResourcesArray, BlitzenEngine::DrawContext& drawContext);

	uint8_t AllocateTextureDescriptorSet(VkDevice device, ROResources& readOnlies, DescriptorContext& descriptorContext);
}