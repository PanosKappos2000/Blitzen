#if defined(linux)

#include <cstring>
#include <stdlib.h>
#include <string.h>
#include "Core/blitMemory.h"
#include "Platform/blitPlatformContext.h"

namespace BlitzenPlatform
{
        /*
            MEMORY
        */
        void* PlatformMalloc(size_t size, uint8_t aligned)
        {
            return malloc(size);
        }

        void PlatformFree(void* pBlock, uint8_t aligned)
        {
            free(pBlock);
        }

        void* PlatformMemZero(void* pBlock, size_t size)
        {
            return memset(pBlock, 0, size);
        }
        void* PlatformMemCopy(void* pDst, void* pSrc, size_t size)
        {
            return memcpy(pDst, pSrc, size);
        }
        void* PlatformMemSet(void* pDst, int32_t value, size_t size)
        {
            return memset(pDst, value, size);
        }
}

#endif