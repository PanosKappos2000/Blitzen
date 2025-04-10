#pragma once

#include "vulkanRenderer.h"

namespace BlitzenVulkan
{
    SurfaceKHR::~SurfaceKHR()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto inst = VulkanRenderer::GetRendererInstance()->m_instance;
            vkDestroySurfaceKHR(inst, handle, nullptr);
        }
    }

    Swapchain::~Swapchain()
    {
        if (swapchainHandle != VK_NULL_HANDLE)
        {
            auto vdv = VulkanRenderer::GetRendererInstance()->m_device;

            for (auto view : swapchainImageViews)
                vkDestroyImageView(vdv, view, nullptr);

            vkDestroySwapchainKHR(vdv, swapchainHandle, nullptr);
        }
    }

    void AllocatedImage::CleanupResources(VmaAllocator allocator, VkDevice device)
    {
        vmaDestroyImage(allocator, image, allocation);
        vkDestroyImageView(device, imageView, nullptr);
    }

    uint8_t PushDescriptorImage::SamplerInit(VkDevice device, VkFilter filter, VkSamplerMipmapMode mipmapMode,
        VkSamplerAddressMode addressMode, void* pNextChain)
    {
        sampler.handle = CreateSampler(device, filter, mipmapMode, addressMode, pNextChain);
        if (sampler.handle == VK_NULL_HANDLE)
            return 0;
        return 1;
    }

    AllocatedImage::~AllocatedImage()
    {
        if (image != VK_NULL_HANDLE)
        {
            auto vma = VulkanRenderer::GetRendererInstance()->m_allocator;
            vmaDestroyImage(vma, image, allocation);
        }

        if (imageView != VK_NULL_HANDLE)
        {
            auto vdv = VulkanRenderer::GetRendererInstance()->m_device;
            vkDestroyImageView(vdv, imageView, nullptr);
        }
    }

    AllocatedBuffer::~AllocatedBuffer()
    {
        if (bufferHandle != VK_NULL_HANDLE)
        {
            auto vma = VulkanRenderer::GetRendererInstance()->m_allocator;
            vmaDestroyBuffer(vma, bufferHandle, allocation);
        }
    }

    PipelineObject::~PipelineObject()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto vdv = VulkanRenderer::GetRendererInstance()->m_device;
            vkDestroyPipeline(vdv, handle, nullptr);
        }
    }

    PipelineLayout::~PipelineLayout()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto vdv = VulkanRenderer::GetRendererInstance()->m_device;
            vkDestroyPipelineLayout(vdv, handle, nullptr);
        }
    }

    ShaderModule::~ShaderModule()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto vdv = VulkanRenderer::GetRendererInstance()->m_device;
            vkDestroyShaderModule(vdv, handle, nullptr);
        }
    }

    DescriptorSetLayout::~DescriptorSetLayout()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto vdv = VulkanRenderer::GetRendererInstance()->m_device;
            vkDestroyDescriptorSetLayout(vdv, handle, nullptr);
        }
    }

    DescriptorPool::~DescriptorPool()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto vdv = VulkanRenderer::GetRendererInstance()->m_device;
            vkDestroyDescriptorPool(vdv, handle, nullptr);
        }
    }

    ImageSampler::~ImageSampler()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto vdv = VulkanRenderer::GetRendererInstance()->m_device;
            vkDestroySampler(vdv, handle, nullptr);
        }
    }

    CommandPool::~CommandPool()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto vdv = VulkanRenderer::GetRendererInstance()->m_device;
            vkDestroyCommandPool(vdv, handle, nullptr);
        }
    }

    Semaphore::~Semaphore()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto vdv = VulkanRenderer::GetRendererInstance()->m_device;
            vkDestroySemaphore(vdv, handle, nullptr);
        }
    }

    SyncFence::~SyncFence()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto vdv = VulkanRenderer::GetRendererInstance()->m_device;
            vkDestroyFence(vdv, handle, nullptr);
        }
    }


    // Acceleration structure is an extensions so it needs to load the destroy function as well
    static void DestroyAccelerationStructureKHR(VkInstance instance, VkDevice device,
        VkAccelerationStructureKHR as, const VkAllocationCallbacks* pAllocator)
    {
        auto func = (PFN_vkDestroyAccelerationStructureKHR)vkGetInstanceProcAddr(
            instance, "vkDestroyAccelerationStructureKHR");
        if (func != nullptr)
        {
            func(device, as, pAllocator);
        }
    }
    AccelerationStructure::~AccelerationStructure()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto vdv = VulkanRenderer::GetRendererInstance()->m_device;
            auto inst = VulkanRenderer::GetRendererInstance()->m_instance;
            DestroyAccelerationStructureKHR(inst, vdv, handle, nullptr);
        }
    }
}