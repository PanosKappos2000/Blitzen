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
}