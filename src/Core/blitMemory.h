#pragma once
#if defined(BLIT_REIN_SANT_ENG)
    #include "Core/blitLogger.h"
    #include "Core/blitAssert.h"
    #include "Engine/blitzenEngine.h"
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

    struct LinearAllocator
    {
        size_t totalAllocated = 0;
        void* pBlock;
        size_t blockSize;
    };

    constexpr size_t LinearAllocatorBlockSize = UINT32_MAX;
    class MemoryManagerState
    {
    public:
        size_t totalAllocated = 0;
        size_t typeAllocations[static_cast<size_t>(AllocationType::MaxTypes)];

        #if defined(BLIT_REQUEST_LINEAR_ALLOCATOR)
            LinearAllocator linearAlloc;
        #endif

        inline static MemoryManagerState* s_pMemoryManager;

        // Defined in blitzenMemory.cpp
        inline MemoryManagerState()
        {
            // Memory manager state allocated on the heap
            s_pMemoryManager = this;
            BlitzenPlatform::PlatformMemZero(s_pMemoryManager, sizeof(MemoryManagerState));

            // Allocate a big block of memory for the linear allocator
            #if defined(BLIT_REQUEST_LINEAR_ALLOCATOR)
                s_pMemoryManager->linearAlloc.blockSize = LinearAllocatorBlockSize;
                s_pMemoryManager->linearAlloc.totalAllocated = 0;
                s_pMemoryManager->linearAlloc.pBlock = 
                BlitAlloc<uint8_t>(BlitzenCore::AllocationType::LinearAlloc, LinearAllocatorBlockSize);
            #endif
        }

        inline ~MemoryManagerState()
        {
            #if defined(BLIT_REQUEST_LINEAR_ALLOCATOR)
                BlitFree<uint8_t>(BlitzenCore::AllocationType::LinearAlloc, 
                linearAlloc.pBlock, linearAlloc.blockSize);
            #endif

            // Warn the user of any memory leaks to look for
            if (totalAllocated)
            {
                #if defined(BLIT_REIN_SANT_ENG)
                BLIT_WARN("There is still unfreed memory, Total: %i \n Unfreed Dynamic Array memory: %i \n Unfreed Hashmap memory: %i \n Unfreed Queue memory: %i \n Unfreed BST memory: %i \n Unfreed String memory: %i \n Unfreed Engine memory: %i \n Unfreed Renderer memory: %i",
                    totalAllocated, typeAllocations[0], typeAllocations[1], typeAllocations[2], typeAllocations[3], typeAllocations[4], typeAllocations[5], typeAllocations[6]
                )
                #else
                printf("There is still unfreed memory, Total: %i \n Unfreed Dynamic Array memory: %i \n Unfreed Hashmap memory: %i \n Unfreed Queue memory: %i \n Unfreed BST memory: %i \n Unfreed String memory: %i \n Unfreed Engine memory: %i \n Unfreed Renderer memory: %i",
                    totalAllocated, typeAllocations[0], typeAllocations[1], typeAllocations[2], typeAllocations[3], typeAllocations[4], typeAllocations[5], typeAllocations[6]);
                #endif
            }
        }

        inline void LogAllocation(AllocationType alloc, size_t size)
        {
            totalAllocated += size;
            typeAllocations[static_cast<uint8_t>(alloc)] += size;
        }

        inline void LogFree(AllocationType alloc, size_t size)
        {
            totalAllocated -= size;
            typeAllocations[static_cast<uint8_t>(alloc)] -= size;
        }

        inline static MemoryManagerState* GetManager() 
        {
            return s_pMemoryManager;
        }
    };

    template<typename T>
    T* BlitAlloc(AllocationType alloc, size_t size)
    {
        MemoryManagerState::GetManager()->LogAllocation(alloc, size * sizeof(T));
        return reinterpret_cast<T*>(BlitzenPlatform::PlatformMalloc(size * sizeof(T), false));
    }

    template<typename T>
    void BlitFree(AllocationType alloc, void* pBlock, size_t size)
    {
        MemoryManagerState::GetManager()->LogFree(alloc, size * sizeof(T));
        BlitzenPlatform::PlatformFree(pBlock, false);
    }

    // Allow call to new with parameter's for the objects constructors. Allocation type is used as a parameter for deduction safety
    template<typename T, typename... P> 
    T* BlitConstructAlloc(AllocationType alloc, const P&... params)
    {
        MemoryManagerState::GetManager()->LogAllocation(alloc, sizeof(T));
        return new T(params...);
    }

    template<typename T, AllocationType alloc>
    T* BlitConstructAlloc(const T& data)
    {
        MemoryManagerState::GetManager()->LogAllocation(alloc, sizeof(T));
        return new T(data);
    }

    template<typename T, AllocationType alloc>
    T* BlitConstructAlloc(T&& data)
    {
        MemoryManagerState::GetManager()->LogAllocation(alloc, sizeof(T));
        return new T(std::move(data));
    }

    // This version takes no parameters
    template<typename T, AllocationType alloc>
    T* BlitConstructAlloc(size_t size)
    {
        MemoryManagerState::GetManager()->LogAllocation(alloc, size * sizeof(T));
        return new T[size];
    }

    // Returns allocated memory of type T, after copyting data from the pointer parameter
    template<typename T, AllocationType alloc>
    T* BlitConstructAlloc(T* pData)
    {
        MemoryManagerState::GetManager()->LogAllocation(alloc, sizeof(T));
        T* res = new T;
        BlitzenPlatform::PlatformMemCopy(res, pData, sizeof(T));
        return res;
    }

    // This free function calls the constructor of the object that gets freed
    template<typename T>
    void BlitDestroyAlloc(AllocationType alloc, T* pToDestroy)
    {
        MemoryManagerState::GetManager()->LogFree(alloc, sizeof(T));
        delete pToDestroy;
    }

    template<typename T>
    void BlitDestroyAlloc(AllocationType alloc, T* pToDestroy, size_t size)
    {
        MemoryManagerState::GetManager()->LogFree(alloc, size * sizeof(T));
        delete [] pToDestroy;
    }

    // The templates below are placeholders to add functionality later
    template<typename T = void>
    void BlitMemCopy(void* pDst, void* pSrc, size_t size)
    {
        BlitzenPlatform::PlatformMemCopy(pDst, pSrc, size);
    }

    // The templates below are placeholders to add functionality later
    template<typename T = void>
    void BlitMemSet(void* pDst, int32_t value, size_t size)
    {
        BlitzenPlatform::PlatformMemSet(pDst, value, size);
    }
    
    // The templates below are placeholders to add functionality later
    template<typename T>
    void BlitZeroMemory(T* pBlock, size_t size = 1)
    {
        BlitzenPlatform::PlatformMemZero(pBlock, sizeof(T) * size);
    }

    // Allocates memory using the linear allocator
    template<typename T>
    void* BlitAllocLinear(AllocationType alloc, size_t size)
    {
        #if defined(BLIT_REQUEST_LINEAR_ALLOCATOR)
            auto pState = BlitzenCore::MemoryManagerState::GetManager();
            auto allocSize = size * sizeof(T);

            if(pState->linearAlloc.totalAllocated + allocSize > pState->linearAlloc.blockSize)
            {
                BLIT_FATAL("Linear allocator depleted, memory not allocated")
                return nullptr;
            }
            void* pBlock = reinterpret_cast<uint8_t*>(
                            pState->linearAlloc.pBlock) 
                            + pState->linearAlloc.totalAllocated;
            pState->linearAlloc.totalAllocated += allocSize;
            return pBlock;
        #else
            return nullptr;
        #endif
    }

}