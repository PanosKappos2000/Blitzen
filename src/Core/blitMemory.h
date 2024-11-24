#pragma once

#include "Core/blitAssert.h"

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
        Engine = 7, 
        Renderer = 8, 
        Entity = 9,
        EntityNode = 10,
        Scene = 11,

        MaxTypes = 12
    };

    /*------------------------------------------------------------------------------------------------------
        This class will handle all allocations for the engine
        I could have avoided making this into a class but, 
        I would then have to keep passing the 2 member variables into each allocation type function call
    ---------------------------------------------------------------------------------------------------------*/
    class MemoryManager
    {
    public:
        MemoryManager();

        void* BlitAlloc(AllocationType alloc, size_t size);
        void BlitFree(AllocationType alloc, void* pBlock, size_t size);
        void  BlitMemCopy(void* pDst, void* pSrc, size_t size);
        void BlitMemSet(void* pDst, int32_t value, size_t size);
        void BlitZeroMemory(void* pBlock, size_t size);

        ~MemoryManager();

    private:

        size_t m_totalAllocated = 0;

        // Keeps track of how much memory has been allocated for each type of allocation
        size_t m_typeAllocations[static_cast<size_t>(AllocationType::MaxTypes)];
    };

    
}