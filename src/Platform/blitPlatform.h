#pragma once 
#include "Core/blitzenEngine.h"

namespace BlitzenPlatform
{
    struct PlatformArgs
    {
        void* m_pPlatform{ nullptr };

        void* SYSTEM{ nullptr };

        void* m_pRenderer{ nullptr };

        void* m_pEditor{ nullptr };
    };

    bool SystemStartup(PlatformArgs& args);

    bool DispatchEvents(void* pPlatform);

    void MakeWindowVisible(BLIT_STRAIGHTHANDLE pPlatform);

    void BlitzenSleep(uint64_t ms);

    void* PlatformMalloc(size_t size, uint8_t aligned);

    void PlatformFree(void* pBlock, uint8_t aligned);

    void* PlatformMemZero(void* pBlock, size_t size);

    void* PlatformMemCopy(void* pDst, void* pSrc, size_t size);

    void* PlatformMemSet(void* pDst, int32_t value, size_t size);
}