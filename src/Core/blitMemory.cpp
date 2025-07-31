#include "blitMemory.h"

namespace BlitzenCore
{
    void BLIT_PTR::Init(size_t size)
    {
        if (!mSize)
        {
            mSize = size;
            mPtr = BlitzenPlatform::PlatformMalloc(size, false);
            LogAllocation(AllocationType::DynamicArray, mSize, AllocationAction::ALLOC);
        }
    }

    BLIT_PTR::~BLIT_PTR()
    {
        if (mSize != 0)
        {
            LogAllocation(AllocationType::DynamicArray, mSize, AllocationAction::FREE);
            BlitzenPlatform::PlatformFree(mPtr, false);
        }
    }

    void BlitReAdjustMemoryAllocation(BLIT_STRAIGHTHANDLE outBlock, size_t newSize, size_t oldSize, AllocationType allocType)
    {
#if defined(BLIT_OFFLINE_BUILD)
        BLIT_STRAIGHTHANDLE pTemp = outBlock;

        // Allocates new block, copies the previous data over, gives the new block pointer to the outBlock
        BLIT_STRAIGHTHANDLE newBlock = MANUAL_ALLOC(allocType, newSize);
        MANUAL_COPY(newBlock, outBlock, oldSize);
        outBlock = newBlock;

        // Frees the old memory
        MANUAL_FREE(allocType, pTemp, oldSize);
#endif
    }
}