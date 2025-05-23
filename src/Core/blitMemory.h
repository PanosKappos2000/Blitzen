#pragma once
#include "Core/blitzenEngine.h"
#if defined(BLIT_REIN_SANT_ENG)
    #include "Core/DbLog/blitLogger.h"
    #include "Core/DbLog/blitAssert.h"
#else
    #include <stdio.h>
#endif
#include <utility>
#include <stdlib.h>

// Platform specific code, needed to allocate on the heap
namespace BlitzenPlatform
{
    // Implementation changes, depending on if the memory manager is used by Blitzen or a different application
    #if defined(BLIT_REIN_SANT_ENG)
        void* PlatformMalloc(size_t size, uint8_t aligned);
        void PlatformFree(void* pBlock, uint8_t aligned);
        void* PlatformMemZero(void* pBlock, size_t size);
        void* PlatformMemCopy(void* pDst, void* pSrc, size_t size);
        void* PlatformMemSet(void* pDst, int32_t value, size_t size);
    #else
        inline void* PlatformMalloc(size_t size, uint8_t aligned)
        {
            return malloc(size);
        }
        inline void PlatformFree(void* pBlock, uint8_t aligned)
        {
            free(pBlock);
        }
        inline void* PlatformMemZero(void* pBlock, size_t size)
        {
            return memset(pBlock, 0, size);
        }
        inline void* PlatformMemCopy(void* pDst, void* pSrc, size_t size)
        {
            return memcpy(pDst, pSrc, size);
        }
        inline void* PlatformMemSet(void* pDst, int32_t value, size_t size)
        {
            return memset(pDst, value, size);
        }
    #endif
}

namespace BlitzenCore
{
    struct LinearAllocator
    {
        size_t m_totalAllocated{ 0 };
        void* m_pBlock{ nullptr };
        size_t m_blockSize{ Ce_LinearAllocatorBlockSize };

        template<typename T>
        T* Alloc(size_t size, AllocationType alloc)
        {
            auto allocSize = size * sizeof(T);
            LogAllocation(alloc, size, AllocationAction::ALLOC);

            if (m_totalAllocated + allocSize > m_blockSize)
            {
                BLIT_FATAL("Linear allocator depleted, memory not allocated");
                return nullptr;
            }

            void* pBlock = reinterpret_cast<T*>(m_pBlock)[m_totalAllocated];
            m_totalAllocated += allocSize;

            return reinterpret_cast<T*>(pBlock);
        }

    };

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
    template<typename T = void>
    void BlitMemCopy(void* pDst, void* pSrc, size_t size)
    {
        BlitzenPlatform::PlatformMemCopy(pDst, pSrc, size);
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