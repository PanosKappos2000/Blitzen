#include "blitMemory.h"
#include "Platform/platform.h"
#include "Engine/blitzenEngine.h"

namespace BlitzenCore
{
    // The global allocation stats have access to every allocation size that has occured
    // When the memory management system shuts down, every allocation should have been freed
    inline AllocationStats globalAllocationStats;

    inline LinearAllocator s_linearAlloc;

    void MemoryManagementInit()
    {
        BlitzenPlatform::PlatformMemZero(&globalAllocationStats, sizeof(AllocationStats));

        // Allocate a big block of memory for the linear allocator
        s_linearAlloc.blockSize = BLIT_LINEAR_ALLOCATOR_MEMORY_BLOCK_SIZE;
        s_linearAlloc.pBlock = BlitAlloc(BlitzenCore::AllocationType::LinearAlloc, BLIT_LINEAR_ALLOCATOR_MEMORY_BLOCK_SIZE);
    }

    void* BlitAlloc(AllocationType alloc, size_t size)
    {
        if(alloc == AllocationType::Unkown || alloc == AllocationType::MaxTypes)
        {
            BLIT_FATAL("Allocation type: %i, A valid allocation type must be specified!", static_cast<uint8_t>(alloc))
        }

        globalAllocationStats.totalAllocated += size;
        globalAllocationStats.typeAllocations[static_cast<size_t>(alloc)] += size;

        return BlitzenPlatform::PlatformMalloc(size, false);
    }

    void BlitFree(AllocationType alloc, void* pBlock, size_t size)
    {
        if(alloc == AllocationType::Unkown || alloc == AllocationType::MaxTypes)
        {
            BLIT_FATAL("Allocation type: %i, A valid allocation type must be specified!", static_cast<uint8_t>(alloc))
        }

        globalAllocationStats.totalAllocated -= size;
        globalAllocationStats.typeAllocations[static_cast<size_t>(alloc)] -= size;

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
        globalAllocationStats.totalAllocated += size;
        globalAllocationStats.typeAllocations[static_cast<size_t>(alloc)] += size;
    }

    void LogFree(AllocationType alloc, size_t size)
    {
        globalAllocationStats.totalAllocated -= size;
        globalAllocationStats.typeAllocations[static_cast<size_t>(alloc)] -= size;
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
        BlitFree(BlitzenCore::AllocationType::LinearAlloc, s_linearAlloc.pBlock, s_linearAlloc.blockSize);

        // Warn the user of any memory leaks to look for
        if (globalAllocationStats.totalAllocated)
        {
            BLIT_WARN("There is still unallocated memory. Total Unallocated Memory: %i \n \
            Unallocated Array Memory: %i \n Unallocated Dynamic Array memory: %i \n Unallocated Hashmap memory: %i \n Unallocated Queue memory: %i \n       \
            Unallocated BST memory: %i \n Unallocated String memory: %i \n Unallocated Engine memory: %i \n Uncallocated Renderer memory: %i \n", \
            globalAllocationStats.totalAllocated, globalAllocationStats.typeAllocations[1], globalAllocationStats.typeAllocations[2], globalAllocationStats.typeAllocations[3], \
            globalAllocationStats.typeAllocations[4], globalAllocationStats.typeAllocations[5], globalAllocationStats.typeAllocations[6], globalAllocationStats.typeAllocations[7], \
            globalAllocationStats.typeAllocations[8])
        }
    }




    void* BlitAllocLinear(AllocationType alloc, size_t size)
    {
        if(s_linearAlloc.totalAllocated + size > s_linearAlloc.blockSize)
        {
            BLIT_WARN("Linear allocator depleted, memory not allocated")
            return nullptr;
        }

        void* pBlock = reinterpret_cast<uint8_t*>(s_linearAlloc.pBlock) + s_linearAlloc.totalAllocated;
        s_linearAlloc.totalAllocated += size;
        return pBlock;
    }
}