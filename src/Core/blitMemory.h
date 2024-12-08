#pragma once

#include "Core/blitAssert.h"

#define BLIT_LINEAR_ALLOCATOR_MEMORY_BLOCK_SIZE         UINT32_MAX

namespace BlitzenCore
{
    enum class AllocationType : uint8_t
    {
        Unkown = 0, 
        Array = 1, 
        DynamicArray = 2,
        Hashmap = 3,
        Queue = 4, 
        Bst = 5,
        String = 6, 
        Engine = 7, 
        Renderer = 8, 
        Entity = 9,
        EntityNode = 10,
        Scene = 11,

        LinearAlloc = 12,

        MaxTypes = 13
    };

    struct AllocationStats
    {
        size_t totalAllocated = 0;

        // Keeps track of how much memory has been allocated for each type of allocation
        size_t typeAllocations[static_cast<size_t>(AllocationType::MaxTypes)];
    };

    void MemoryManagementInit();

    void* BlitAlloc(AllocationType alloc, size_t size);
    void BlitFree(AllocationType alloc, void* pBlock, size_t size);
    void  BlitMemCopy(void* pDst, void* pSrc, size_t size);
    void BlitMemSet(void* pDst, int32_t value, size_t size);
    void BlitZeroMemory(void* pBlock, size_t size);

    void MemoryManagementShutdown();

    struct LinearAllocator
    {
        size_t totalAllocated = 0;
        void* pBlock;
        size_t blockSize;
    };

    void* BlitAllocLinear(AllocationType alloc, size_t size);
}