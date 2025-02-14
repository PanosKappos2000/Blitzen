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
        SmartPointer = 12,

        LinearAlloc = 13,

        MaxTypes = 14
    };

    // Log all allocations to catch memory leaks
    void LogAllocation(AllocationType alloc, size_t size);
    // Unlog allocations when freed, to catch memory leaks
    void LogFree(AllocationType alloc, size_t size);

    template<typename T>
    T* BlitAlloc(AllocationType alloc, size_t size)
    {
        BLIT_ASSERT(alloc != AllocationType::Unkown && alloc != AllocationType::MaxTypes)

        LogAllocation(alloc, size * sizeof(T));

        return reinterpret_cast<T*>(BlitzenPlatform::PlatformMalloc(size * sizeof(T), false));
    }

    template<typename T>
    void BlitFree(AllocationType alloc, void* pBlock, size_t size)
    {
        BLIT_ASSERT(alloc != AllocationType::Unkown && alloc != AllocationType::MaxTypes)

        LogFree(alloc, size * sizeof(T));

        BlitzenPlatform::PlatformFree(pBlock, false);
    }

    // This allocation function calls the constructor of the object that gets allocated(the constructor must have no parameters)
    template<typename T, typename... P> 
    T* BlitConstructAlloc(AllocationType alloc, P&... params)
    {
        LogAllocation(alloc, sizeof(T));
        return new T(params...);
    }

    template<typename T, AllocationType A>
    T* BlitConstructAlloc(const T& data)
    {
        LogAllocation(A, sizeof(T));
        return new T(data);
    }

    template<typename T, AllocationType A>
    T* BlitConstructAlloc(T&& data)
    {
        LogAllocation(A, sizeof(T));
        return new T(std::move(data));
    }

    // This version takes no parameters
    template<typename T, AllocationType A>
    T* BlitConstructAlloc(size_t size)
    {
        LogAllocation(A, size * sizeof(T));
        return new T[size];
    }

    // This free function calls the constructor of the object that gets freed
    template<typename T>
    void BlitDestroyAlloc(AllocationType alloc, T* pToDestroy)
    {
        LogFree(alloc, sizeof(T));
        delete pToDestroy;
    }

    // Allocates memory using the linear allocator
    void* BlitAllocLinear(AllocationType alloc, size_t size);

    void  BlitMemCopy(void* pDst, void* pSrc, size_t size);
    void BlitMemSet(void* pDst, int32_t value, size_t size);
    void BlitZeroMemory(void* pBlock, size_t size);
}

namespace BlitzenPlatform
{
    // Called only by the memory manager
    void* PlatformMalloc(size_t size, uint8_t aligned);
    void PlatformFree(void* pBlock, uint8_t aligned);
    void* PlatformMemZero(void* pBlock, size_t size);
    void* PlatformMemCopy(void* pDst, void* pSrc, size_t size);
    void* PlatformMemSet(void* pDst, int32_t value, size_t size);
}