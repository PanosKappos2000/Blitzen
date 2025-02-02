#include "blitMemory.h"
#include "Platform/platform.h"
#include "Engine/blitzenEngine.h"
#include "Core/blitzenCore.h"

namespace BlitzenCore
{
    // The global allocation stats have access to every allocation size that has occured
    // When the memory management system shuts down, every allocation should have been freed
    inline MemoryManagerState* inl_memoryManagerState = nullptr;

    void MemoryManagementInit(void* pState)
    {
        inl_memoryManagerState = reinterpret_cast<MemoryManagerState*>(pState);

        // Allocate a big block of memory for the linear allocator
        inl_memoryManagerState->linearAlloc.blockSize = BLIT_LINEAR_ALLOCATOR_MEMORY_BLOCK_SIZE;
        inl_memoryManagerState->linearAlloc.totalAllocated = 0;
        inl_memoryManagerState->linearAlloc.pBlock = BlitAlloc(BlitzenCore::AllocationType::LinearAlloc, BLIT_LINEAR_ALLOCATOR_MEMORY_BLOCK_SIZE);
    }

    BlitzenVulkan::MemoryCrucialHandles* GetVulkanMemoryCrucials()
    {
        return &inl_memoryManagerState->vkCrucial;
    }

    void* BlitAlloc(AllocationType alloc, size_t size)
    {
        if(alloc == AllocationType::Unkown || alloc == AllocationType::MaxTypes)
        {
            BLIT_FATAL("Allocation type: %i, A valid allocation type must be specified!", static_cast<uint8_t>(alloc))
        }

        inl_memoryManagerState->linearAlloc.totalAllocated += size;
        inl_memoryManagerState->typeAllocations[static_cast<size_t>(alloc)] += size;

        return BlitzenPlatform::PlatformMalloc(size, false);
    }

    void BlitFree(AllocationType alloc, void* pBlock, size_t size)
    {
        if(alloc == AllocationType::Unkown || alloc == AllocationType::MaxTypes)
        {
            BLIT_FATAL("Allocation type: %i, A valid allocation type must be specified!", static_cast<uint8_t>(alloc))
        }

        inl_memoryManagerState->totalAllocated -= size;
        inl_memoryManagerState->typeAllocations[static_cast<size_t>(alloc)] -= size;

        BlitzenPlatform::PlatformFree(pBlock, false);
    }

    void BlitMemCopy(void* pDst, void* pSrc, size_t size)
    {
        BlitzenPlatform::PlatformMemCopy(pDst, pSrc, size);
    }

    void BlitMemSet(void* pDst, int32_t value, size_t size)
    {
        BlitzenPlatform::PlatformMemSet(pDst, value, size);
    }
    
    void BlitZeroMemory(void* pBlock, size_t size)
    {
        BlitzenPlatform::PlatformMemZero(pBlock, size);
    }

    void LogAllocation(AllocationType alloc, size_t size)
    {
        inl_memoryManagerState->totalAllocated += size;
        inl_memoryManagerState->typeAllocations[static_cast<size_t>(alloc)] += size;
    }

    void LogFree(AllocationType alloc, size_t size)
    {
        inl_memoryManagerState->totalAllocated -= size;
        inl_memoryManagerState->typeAllocations[static_cast<size_t>(alloc)] -= size;
    }

    void MemoryManagementShutdown()
    {
        // Memory management should not shut down before the Engine
        if (BlitzenEngine::Engine::GetEngineInstancePointer())
        {
            BLIT_ERROR("Blitzen is still active, memory management cannot be shutdown")
            return;
        }

        // Free the big block of memory held by the linear allocator
        BlitFree(BlitzenCore::AllocationType::LinearAlloc, inl_memoryManagerState->linearAlloc.pBlock, 
        inl_memoryManagerState->linearAlloc.blockSize);

        // Warn the user of any memory leaks to look for
        if (inl_memoryManagerState->totalAllocated)
        {
            BLIT_WARN("There is still unallocated memory. Total Unallocated Memory: %i \n \
            Unallocated Array Memory: %i \n \
            Unallocated Dynamic Array memory: %i \n \
            Unallocated Hashmap memory: %i \n Unallocated Queue memory: %i \n \
            Unallocated BST memory: %i \n \
            Unallocated String memory: %i \n \
            Unallocated Engine memory: %i \n \
            Uncallocated Renderer memory: %i \n", \
            inl_memoryManagerState->totalAllocated, 
            inl_memoryManagerState->typeAllocations[1], 
            inl_memoryManagerState->typeAllocations[2], 
            inl_memoryManagerState->typeAllocations[3], 
            inl_memoryManagerState->typeAllocations[4], 
            inl_memoryManagerState->typeAllocations[5], 
            inl_memoryManagerState->typeAllocations[6], 
            inl_memoryManagerState->typeAllocations[7],
            inl_memoryManagerState->typeAllocations[8])
        }
    }

    void* BlitAllocLinear(AllocationType alloc, size_t size)
    {
        if(inl_memoryManagerState->linearAlloc.totalAllocated + size > 
        inl_memoryManagerState->linearAlloc.blockSize)
        {
            BLIT_FATAL("Linear allocator depleted, memory not allocated")
            return nullptr;
        }

        void* pBlock = reinterpret_cast<uint8_t*>(inl_memoryManagerState->linearAlloc.pBlock) + 
        inl_memoryManagerState->linearAlloc.totalAllocated;
        inl_memoryManagerState->linearAlloc.totalAllocated += size;
        return pBlock;
    }
}