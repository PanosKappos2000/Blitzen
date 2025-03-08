#pragma once

#include "Core/blitLogger.h"
#include "Core/blitAssert.h"
#include <utility>

// Platform specific code, needed to allocate on the heap
namespace BlitzenPlatform
{
    // Called only by the memory manager
    void* PlatformMalloc(size_t size, uint8_t aligned);
    void PlatformFree(void* pBlock, uint8_t aligned);
    void* PlatformMemZero(void* pBlock, size_t size);
    void* PlatformMemCopy(void* pDst, void* pSrc, size_t size);
    void* PlatformMemSet(void* pDst, int32_t value, size_t size);
}

namespace BlitzenCore
{
    enum class AllocationType : uint8_t
    {  
        DynamicArray = 0,
        Hashmap = 1,
        Queue = 2, 
        Bst = 3,
        String = 4, 
        Engine = 5, 
        Renderer = 6, 
        Entity = 7,
        EntityNode = 8,
        Scene = 9,
        SmartPointer = 10,
        LinearAlloc = 11,

        MaxTypes = 12
    };

    constexpr size_t ce_linearAllocatorBlockSize = UINT32_MAX;

    // Log all allocations to catch memory leaks
    void LogAllocation(AllocationType alloc, size_t size);
    // Unlog allocations when freed, to catch memory leaks
    void LogFree(AllocationType alloc, size_t size);

    template<typename T>
    T* BlitAlloc(AllocationType alloc, size_t size)
    {
        BLIT_ASSERT(alloc != AllocationType::MaxTypes)
        LogAllocation(alloc, size * sizeof(T));
        return reinterpret_cast<T*>(BlitzenPlatform::PlatformMalloc(size * sizeof(T), false));
    }

    template<typename T>
    void BlitFree(AllocationType alloc, void* pBlock, size_t size)
    {
        BLIT_ASSERT(alloc != AllocationType::MaxTypes)
        LogFree(alloc, size * sizeof(T));
        BlitzenPlatform::PlatformFree(pBlock, false);
    }

    // Allow call to new with parameter's for the objects constructors. Allocation type is used as a parameter for deduction safety
    template<typename T, typename... P> 
    T* BlitConstructAlloc(AllocationType alloc, const P&... params)
    {
        LogAllocation(alloc, sizeof(T));
        return new T(params...);
    }

    template<typename T, AllocationType alloc>
    T* BlitConstructAlloc(const T& data)
    {
        LogAllocation(alloc, sizeof(T));
        return new T(data);
    }

    template<typename T, AllocationType alloc>
    T* BlitConstructAlloc(T&& data)
    {
        LogAllocation(alloc, sizeof(T));
        return new T(std::move(data));
    }

    // This version takes no parameters
    template<typename T, AllocationType alloc>
    T* BlitConstructAlloc(size_t size)
    {
        LogAllocation(alloc, size * sizeof(T));
        return new T[size];
    }

    // Returns allocated memory of type T, after copyting data from the pointer parameter
    template<typename T, AllocationType alloc>
    T* BlitConstructAlloc(T* pData)
    {
        LogAllocation(alloc, sizeof(T));
        T* res = new T;
        BlitzenPlatform::PlatformMemCopy(res, pData, sizeof(T));
        return res;
    }

    // This free function calls the constructor of the object that gets freed
    template<typename T>
    void BlitDestroyAlloc(AllocationType alloc, T* pToDestroy)
    {
        LogFree(alloc, sizeof(T));
        delete pToDestroy;
    }

    template<typename T>
    void BlitDestroyAlloc(AllocationType alloc, T* pToDestroy, size_t size)
    {
        LogFree(alloc, size * sizeof(T));
        delete [] pToDestroy;
    }

    // Allocates memory using the linear allocator
    void* BlitAllocLinear(AllocationType alloc, size_t size);

    void  BlitMemCopy(void* pDst, void* pSrc, size_t size);
    void BlitMemSet(void* pDst, int32_t value, size_t size);
    void BlitZeroMemory(void* pBlock, size_t size);
}