#include "blitMemory.h"
#include "Platform/platform.h"
#include "mainEngine.h"

namespace BlitzenCore
{
    static AllocationStats globalAllocationStats;

    void MemoryManagementInit()
    {
        BlitzenPlatform::PlatformMemZero(&globalAllocationStats, sizeof(AllocationStats));
    }

    void* BlitAlloc(AllocationType alloc, size_t size)
    {
        // This might need to be an assertion so that the application fails, when memory is mihandled
        if(alloc == AllocationType::Unkown || alloc == AllocationType::MaxTypes)
        {
            FATAL_MESSAGE("Allocation type: %i, A valid allocation type must be specified!", static_cast<uint8_t>(alloc))
        }

        globalAllocationStats.totalAllocated += size;
        globalAllocationStats.typeAllocations[static_cast<size_t>(alloc)] += size;

        return BlitzenPlatform::PlatformMalloc(size, false);
    }

    void BlitFree(AllocationType alloc, void* pBlock, size_t size)
    {
        // This might need to be an assertion so that the application fails, when memory is mihandled
        if(alloc == AllocationType::Unkown || alloc == AllocationType::MaxTypes)
        {
            FATAL_MESSAGE("Allocation type: %i, A valid allocation type must be specified!", static_cast<uint8_t>(alloc))
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
        BLIT_ASSERT_MESSAGE(!(BlitzenEngine::Engine::GetEngineInstancePointer()), "Blitzen is still active, memory management cannot be shutdown")
        BLIT_ASSERT_MESSAGE(!globalAllocationStats.totalAllocated, "There is still unallocated memory")
    }
}