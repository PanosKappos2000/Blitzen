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
        BLIT_ASSERT_MESSAGE(!globalAllocationStats.totalAllocated, "There is still unallocated memory")
    }
}