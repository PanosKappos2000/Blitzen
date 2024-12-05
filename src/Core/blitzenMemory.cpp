#include "blitMemory.h"
#include "Platform/platform.h"
#include "mainEngine.h"

namespace BlitzenCore
{
    // The global allocation stats have access to every allocation size that has occured
    // When the memory management system shuts down, every allocation should have been freed
    static AllocationStats globalAllocationStats;

    void MemoryManagementInit()
    {
        BlitzenPlatform::PlatformMemZero(&globalAllocationStats, sizeof(AllocationStats));
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

    void MemoryManagementShutdown()
    {
        if (BlitzenEngine::Engine::GetEngineInstancePointer())
        {
            BLIT_ERROR("Blitzen is still active, memory management cannot be shutdown")
            return;
        }
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
}