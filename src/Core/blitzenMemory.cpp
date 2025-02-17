#include "blitMemory.h"
#include "Platform/platform.h"
#include "Engine/blitzenEngine.h"
#include "Core/blitzenCore.h"

#define GET_BLITZEN_MEMORY_MANAGER_STATE() BlitzenCore::MemoryManagerState::GetManager();

namespace BlitzenCore
{
    MemoryManagerState* MemoryManagerState::s_pMemoryManager;

    MemoryManagerState::MemoryManagerState()
    {
        // Memory manager state allocated on the heap
        s_pMemoryManager = this;
        BlitzenPlatform::PlatformMemZero(s_pMemoryManager, sizeof(MemoryManagerState));

        // Allocate a big block of memory for the linear allocator
        s_pMemoryManager->linearAlloc.blockSize = ce_linearAllocatorBlockSize;
        s_pMemoryManager->linearAlloc.totalAllocated = 0;
        s_pMemoryManager->linearAlloc.pBlock = 
        BlitAlloc<uint8_t>(BlitzenCore::AllocationType::LinearAlloc, ce_linearAllocatorBlockSize);
    }

    BlitzenVulkan::MemoryCrucialHandles* GetVulkanMemoryCrucials()
    {
        #ifdef BLITZEN_VULKAN
            MemoryManagerState* pState = GET_BLITZEN_MEMORY_MANAGER_STATE();
            return &(pState->vkCrucial);
        #else
            return nullptr;
        #endif
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

    void LogAllocation(AllocationType alloc, size_t size)
    {
        MemoryManagerState* pState = GET_BLITZEN_MEMORY_MANAGER_STATE();

        pState->totalAllocated += size;
        pState->typeAllocations[static_cast<size_t>(alloc)] += size;
    }

    void LogFree(AllocationType alloc, size_t size)
    {
        MemoryManagerState* pState = GET_BLITZEN_MEMORY_MANAGER_STATE();

        pState->totalAllocated -= size;
        pState->typeAllocations[static_cast<size_t>(alloc)] -= size;
    }

    MemoryManagerState::~MemoryManagerState()
    {
        // Memory management should not shut down before the Engine
        if (BlitzenEngine::Engine::GetEngineInstancePointer())
        {
            BLIT_ERROR("Blitzen is still active, memory management cannot be shutdown")
            return;
        }

        MemoryManagerState* pState = GET_BLITZEN_MEMORY_MANAGER_STATE();

        BLIT_ASSERT(pState)

        // Free the big block of memory held by the linear allocator
        BlitFree<uint8_t>(BlitzenCore::AllocationType::LinearAlloc, pState->linearAlloc.pBlock, pState->linearAlloc.blockSize);

        // Warn the user of any memory leaks to look for
        if (pState->totalAllocated)
        {
            BLIT_WARN("There is still unallocated memory. Total Unallocated Memory: %i \n \
            Unallocated Array Memory: %i \n \
            Unallocated Dynamic Array memory: %i \n \
            Unallocated Hashmap memory: %i \n Unallocated Queue memory: %i \n \
            Unallocated BST memory: %i \n \
            Unallocated String memory: %i \n \
            Unallocated Engine memory: %i \n \
            Uncallocated Renderer memory: %i \n", \
            pState->totalAllocated, 
            pState->typeAllocations[1], 
            pState->typeAllocations[2], 
            pState->typeAllocations[3], 
            pState->typeAllocations[4], 
            pState->typeAllocations[5], 
            pState->typeAllocations[6], 
            pState->typeAllocations[7],
            pState->typeAllocations[8])
        }
    }

    void* BlitAllocLinear(AllocationType alloc, size_t size)
    {
        MemoryManagerState* pState = GET_BLITZEN_MEMORY_MANAGER_STATE();

        if(pState->linearAlloc.totalAllocated + size > pState->linearAlloc.blockSize)
        {
            BLIT_FATAL("Linear allocator depleted, memory not allocated")
            return nullptr;
        }

        void* pBlock = reinterpret_cast<uint8_t*>(pState->linearAlloc.pBlock) + pState->linearAlloc.totalAllocated;
        pState->linearAlloc.totalAllocated += size;
        return pBlock;
    }
}