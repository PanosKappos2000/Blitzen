#pragma once

#include "Core/blitAssert.h"

#define BLIT_LINEAR_ALLOCATOR_MEMORY_BLOCK_SIZE         UINT32_MAX * 4

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
        SmartPointer = 12,

        LinearAlloc = 13,

        MaxTypes = 14
    };

    // This is used to log every allocation and check if there are any memory leaks in the end
    struct AllocationStats
    {
        size_t totalAllocated = 0;

        // Keeps track of how much memory has been allocated for each type of allocation
        size_t typeAllocations[static_cast<size_t>(AllocationType::MaxTypes)];
    };

    // Initializes the different allocators and the allocation stats
    void MemoryManagementInit();

    void* BlitAlloc(AllocationType alloc, size_t size);
    void BlitFree(AllocationType alloc, void* pBlock, size_t size);
    void  BlitMemCopy(void* pDst, void* pSrc, size_t size);
    void BlitMemSet(void* pDst, int32_t value, size_t size);
    void BlitZeroMemory(void* pBlock, size_t size);

    // Log all allocations to catch memory leaks
    void LogAllocation(AllocationType alloc, size_t size);
    // Unlog allocations when freed, to catch memory leaks
    void LogFree(AllocationType alloc, size_t size);

    // This allocation function calls the constructor of the object that gets allocated(the constructor must have no parameters)
    template<typename T> 
    T* BlitConstructAlloc(AllocationType alloc)
    {
        LogAllocation(alloc, sizeof(T));
        return new T();
    }

    // This free function calls the constructor of the object that get freed
    template<typename T>
    void BlitDestroyAlloc(AllocationType alloc, T* pToDestroy)
    {
        LogFree(alloc, sizeof(T));
        delete pToDestroy;
    }

    void MemoryManagementShutdown();

    // The linear allocator allocates a big amount of memory on boot and places everything it allocates there
    // It deallocates the whole thing when memory management is shutdown
    struct LinearAllocator
    {
        size_t totalAllocated = 0;
        void* pBlock;
        size_t blockSize;
    };

    // Allocates memory using the linear allocator
    void* BlitAllocLinear(AllocationType alloc, size_t size);
}