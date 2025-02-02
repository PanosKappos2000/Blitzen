#pragma once

#include "Core/blitLogger.h"
#include "BlitzenVulkan/vulkanRenderer.h"

namespace BlitzenCore
{
    // The linear allocator allocates a big amount of memory on boot and places everything it allocates there
    // It deallocates the whole thing when memory management is shutdown
    struct LinearAllocator
    {
        size_t totalAllocated = 0;
        void* pBlock;
        size_t blockSize;
    };

    // This is used to log every allocation and check if there are any memory leaks in the end
    struct MemoryManagerState
    {
        size_t totalAllocated = 0;

        // Keeps track of how much memory has been allocated for each type of allocation
        size_t typeAllocations[static_cast<size_t>(AllocationType::MaxTypes)];

        LinearAllocator linearAlloc;

        // These exist here so that they are destroyed after Vulkan
        BlitzenVulkan::MemoryCrucialHandles vkCrucial;
    };
}