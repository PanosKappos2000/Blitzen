#include "blitMemory.h"
#include "Platform/platform.h"

namespace BlitzenCore
{
    MemoryManager::MemoryManager()
    {
        // Zero out the memory held by allocation types
        BlitzenPlatform::PlatformMemZero(&(m_typeAllocations), sizeof(size_t) * static_cast<size_t>(AllocationType::MaxTypes));
    }

    void* MemoryManager::BlitAlloc(AllocationType alloc, size_t size)
    {
        // This might need to be an assertion so that the application fails, when memory is mihandled
        if(alloc == AllocationType::Unkown || alloc == AllocationType::MaxTypes)
        {
            FATAL_MESSAGE("Allocation type: %i, A valid allocation type must be specified!", static_cast<uint8_t>(alloc))
        }

        m_totalAllocated += size;
        m_typeAllocations[static_cast<size_t>(alloc)] += size;

        return BlitzenPlatform::PlatformMalloc(size, false);
    }

    void MemoryManager::BlitFree(AllocationType alloc, void* pBlock, size_t size)
    {
        // This might need to be an assertion so that the application fails, when memory is mihandled
        if(alloc == AllocationType::Unkown || alloc == AllocationType::MaxTypes)
        {
            FATAL_MESSAGE("Allocation type: %i, A valid allocation type must be specified!", static_cast<uint8_t>(alloc))
        }

        m_totalAllocated -= size;
        m_typeAllocations[static_cast<size_t>(alloc)] -= size;

        BlitzenPlatform::PlatformFree(pBlock, false);
    }

    void MemoryManager::BlitMemCopy(void* pDst, void* pSrc, size_t size)
    {
        BlitzenPlatform::PlatformMemCopy(pDst, pSrc, size);
    }

    void MemoryManager::BlitMemSet(void* pDst, int32_t value, size_t size)
    {
        BlitzenPlatform::PlatformMemSet(pDst, value, size);
    }
    
    void MemoryManager::BlitZeroMemory(void* pBlock, size_t size)
    {
        BlitzenPlatform::PlatformMemZero(pBlock, size);
    }



    MemoryManager::~MemoryManager()
    {
        if(m_totalAllocated > 0)
        {
            ERROR_MESSAGE("Some allocation were not freed, allocated memory size: %i", m_totalAllocated)
        }
    }
}