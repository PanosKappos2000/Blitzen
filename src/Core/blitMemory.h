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

    // This allocation actually calls the constructor of the object that gets allocated(the constructor must have no parameters)
    template<typename T> 
    T* BlitConstructAlloc(AllocationType alloc)
    {
        LogAllocation(alloc, sizeof(T));
        return new T();
    }
    // The free for the above version of free
    template<typename T>
    void BlitDestroyAlloc(AllocationType alloc, T* pToDestroy)
    {
        LogFree(alloc, sizeof(T));
        delete pToDestroy;
    }
    // Helper for the above functions since they are templated and need to be defined here and have no access to the static memory stats
    void LogAllocation(AllocationType alloc, size_t size);
    void LogFree(AllocationType alloc, size_t size);

    void MemoryManagementShutdown();

    struct LinearAllocator
    {
        size_t totalAllocated = 0;
        void* pBlock;
        size_t blockSize;
    };

    void* BlitAllocLinear(AllocationType alloc, size_t size);
}