#pragma once
#include "Core/blitzenEngine.h"
#include "Platform/blitPlatform.h"
#include <utility>
#include <stdlib.h>
#if !defined(BLIT_REIN_SANT_ENG)
#include "BlitCL/platform.h"
#endif

namespace BlitzenCore
{
    inline void* MANUAL_ALLOC(AllocationType alloc, size_t size)
    {
        LogAllocation(alloc, size, AllocationAction::ALLOC);

        return BlitzenPlatform::PlatformMalloc(size , false);
    }

    inline void MANUAL_FREE(AllocationType alloc, void* pBlock, size_t size)
    {
        LogAllocation(alloc, size, AllocationAction::FREE);

        BlitzenPlatform::PlatformFree(pBlock, false);
    }

    inline void MANUAL_COPY(void* pDst, void* pSrc, size_t size)
    {
        BlitzenPlatform::PlatformMemCopy(pDst, pSrc, size);
    }

    inline void BlitMemCopy(void* pDst, void* pSrc, size_t size)
    {
        BlitzenPlatform::PlatformMemCopy(pDst, pSrc, size);
    }

    template<typename T>
    T* BlitAlloc(AllocationType alloc, size_t size)
    {
        LogAllocation(alloc, size * sizeof(T), AllocationAction::ALLOC);

        return reinterpret_cast<T*>(BlitzenPlatform::PlatformMalloc(size * sizeof(T), false));
    }

    template<typename T>
    void BlitFree(AllocationType alloc, void* pBlock, size_t size)
    {
        LogAllocation(alloc, size * sizeof(T), AllocationAction::FREE);

        BlitzenPlatform::PlatformFree(pBlock, false);
    }

    // Allow call to new with parameter's for the objects constructors. Allocation type is used as a parameter for deduction safety
    template<class T, typename... ARGS> 
    T* BlitConstructAlloc(AllocationType alloc, ARGS&&... params)
    {
        LogAllocation(alloc, sizeof(T), AllocationAction::ALLOC);

        return new T(std::forward<ARGS>(params)...);
    }

    template<typename T, AllocationType alloc>
    T* BlitConstructAlloc(const T& data)
    {
        LogAllocation(alloc, sizeof(T), AllocationAction::ALLOC);

        return new T(data);
    }

    template<typename T, AllocationType alloc>
    T* BlitConstructAlloc(T&& data)
    {
        LogAllocation(alloc, sizeof(T), AllocationAction::ALLOC);

        return new T(std::move(data));
    }

    // This version takes no parameters
    template<typename T, AllocationType alloc>
    T* BlitConstructAlloc(size_t size)
    {
        LogAllocation(alloc, size * sizeof(T), AllocationAction::ALLOC);

        return new T[size];
    }

    // Returns allocated memory of type T, after copyting data from the pointer parameter
    template<typename T, AllocationType alloc>
    T* BlitConstructAlloc(T* pData)
    {
        LogAllocation(alloc, sizeof(T), AllocationAction::ALLOC);

        T* res = new T;
        BlitzenPlatform::PlatformMemCopy(res, pData, sizeof(T));
        return res;
    }

    // This free function calls the constructor of the object that gets freed
    template<typename T>
    void BlitDestroyAlloc(AllocationType alloc, T* pToDestroy)
    {
        LogAllocation(alloc, sizeof(T), AllocationAction::FREE);

        delete pToDestroy;
    }

    template<typename T>
    void BlitDestroyAlloc(AllocationType alloc, T* pToDestroy, size_t size)
    {
        LogAllocation(alloc, size * sizeof(T), AllocationAction::FREE);

        delete [] pToDestroy;
    }

    // The templates below are placeholders to add functionality later
    template<typename T>
    void BlitMemSet(T* pDst, int32_t value, size_t size)
    {
        BlitzenPlatform::PlatformMemSet(pDst, value, sizeof(T) * size);
    }
    
    // The templates below are placeholders to add functionality later
    template<typename T>
    void BlitZeroMemory(T* pBlock, size_t size = 1)
    {
        BlitzenPlatform::PlatformMemZero(pBlock, sizeof(T) * size);
    }

}