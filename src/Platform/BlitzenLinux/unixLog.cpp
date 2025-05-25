#if defined(linux)

#include "Core/blitzenEngine.h"
#include "Platform/blitPlatformContext.h"
#include <stdio.h>

namespace BlitzenPlatform
{
    void PlatformConsoleWrite(const char* message, uint8_t color)
    {
        printf("\033[%sm%s\033[0m", BlitzenCore::CE_UNIX_CONSOLE_LOGGER_COLORS[color], message);
    }

    void PlatformConsoleError(const char* message, uint8_t color)
    {
        printf("\033[%sm%s\033[0m", BlitzenCore::CE_UNIX_CONSOLE_LOGGER_COLORS[color], message);
    }
}

#endif