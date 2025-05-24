#include "vulkanRenderer.h"

namespace BlitzenVulkan
{
    static MemoryCrucialHandles* S_GET_VULKAN_MEMORY(MemoryCrucialHandles* pHandles = nullptr)
    {
        static MemoryCrucialHandles* s_pMemoryHandles{ pHandles };
        return s_pMemoryHandles;
    }

    MemoryCrucialHandles* InitMemoryCrucialHandles(MemoryCrucialHandles* pHandles)
    {
		return S_GET_VULKAN_MEMORY(pHandles);
    }

    SurfaceKHR::~SurfaceKHR()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto inst = S_GET_VULKAN_MEMORY()->instance;

            vkDestroySurfaceKHR(inst, handle, nullptr);
        }
    }

    Swapchain::~Swapchain()
    {
        if (swapchainHandle != VK_NULL_HANDLE)
        {
            auto vdv = S_GET_VULKAN_MEMORY()->device;

            for (auto view : swapchainImageViews)
            {
                vkDestroyImageView(vdv, view, nullptr);
            }

            vkDestroySwapchainKHR(vdv, swapchainHandle, nullptr);
        }
    }

    void AllocatedImage::CleanupResources(VmaAllocator allocator, VkDevice device)
    {
        vmaDestroyImage(allocator, image, allocation);
        vkDestroyImageView(device, imageView, nullptr);
    }

    AllocatedImage::~AllocatedImage()
    {
        if (image != VK_NULL_HANDLE)
        {
            auto vma = S_GET_VULKAN_MEMORY()->allocator;
            vmaDestroyImage(vma, image, allocation);
        }

        if (imageView != VK_NULL_HANDLE)
        {
            auto vdv = S_GET_VULKAN_MEMORY()->device;
            vkDestroyImageView(vdv, imageView, nullptr);
        }
    }

    AllocatedBuffer::~AllocatedBuffer()
    {
        if (bufferHandle != VK_NULL_HANDLE)
        {
            auto vma = S_GET_VULKAN_MEMORY()->allocator;
            vmaDestroyBuffer(vma, bufferHandle, allocation);
        }
    }

    PipelineObject::~PipelineObject()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto vdv = S_GET_VULKAN_MEMORY()->device;
            vkDestroyPipeline(vdv, handle, nullptr);
        }
    }

    PipelineLayout::~PipelineLayout()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto vdv = S_GET_VULKAN_MEMORY()->device;
            vkDestroyPipelineLayout(vdv, handle, nullptr);
        }
    }

    ShaderModule::~ShaderModule()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto vdv = S_GET_VULKAN_MEMORY()->device;
            vkDestroyShaderModule(vdv, handle, nullptr);
        }
    }

    DescriptorSetLayout::~DescriptorSetLayout()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto vdv = S_GET_VULKAN_MEMORY()->device;
            vkDestroyDescriptorSetLayout(vdv, handle, nullptr);
        }
    }

    DescriptorPool::~DescriptorPool()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto vdv = S_GET_VULKAN_MEMORY()->device;
            vkDestroyDescriptorPool(vdv, handle, nullptr);
        }
    }

    ImageSampler::~ImageSampler()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto vdv = S_GET_VULKAN_MEMORY()->device;
            vkDestroySampler(vdv, handle, nullptr);
        }
    }

    CommandPool::~CommandPool()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto vdv = S_GET_VULKAN_MEMORY()->device;
            vkDestroyCommandPool(vdv, handle, nullptr);
        }
    }

    Semaphore::~Semaphore()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto vdv = S_GET_VULKAN_MEMORY()->device;
            vkDestroySemaphore(vdv, handle, nullptr);
        }
    }

    SyncFence::~SyncFence()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto vdv = S_GET_VULKAN_MEMORY()->device;
            vkDestroyFence(vdv, handle, nullptr);
        }
    }


    // Acceleration structure is an extensions so it needs to load the destroy function as well
    static void DestroyAccelerationStructureKHR(VkInstance instance, VkDevice device, VkAccelerationStructureKHR as, const VkAllocationCallbacks* pAllocator)
    {
        auto func = (PFN_vkDestroyAccelerationStructureKHR)vkGetInstanceProcAddr(instance, "vkDestroyAccelerationStructureKHR");
        if (func != nullptr)
        {
            func(device, as, pAllocator);
        }
    }
    AccelerationStructure::~AccelerationStructure()
    {
        if (handle != VK_NULL_HANDLE)
        {
            auto vdv = S_GET_VULKAN_MEMORY()->device;
            auto inst = S_GET_VULKAN_MEMORY()->instance;
            DestroyAccelerationStructureKHR(inst, vdv, handle, nullptr);
        }
    }
}