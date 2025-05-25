#if defined(linux)

#include "Core/blitzenEngine.h"
#include "Platform/blitPlatformContext.h"
#include <stdio.h>
#include "Platform/Common/blitMappedFile.h"

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

    void PlatformLoggerFileWrite(const char* message, uint8_t color)
    {
        static MEMORY_MAPPED_FILE_SCOPE s_scopedFile;

        if (s_scopedFile.Failed())
        {
            auto mmfResult{ s_scopedFile.OpenWrite("blitLogOutput.txt", BlitzenCore::Ce_BlitLogOutputFileSize) };
            if (mmfResult != BLIT_MMF_RES::SUCCESS && mmfResult != BLIT_MMF_RES::SUCCESS_FALLBACK)
            {
                const char* mmfErrorString{ GET_BLIT_MMF_RES_ERROR_STR(mmfResult) };
                PlatformConsoleError(mmfErrorString, (uint8_t)BlitzenCore::LogLevel::Error);
                PlatformConsoleWrite(message, color);
                return;
            }
        }

        WriteMemoryMappedFile(s_scopedFile, s_scopedFile.m_endOffset, strlen(message), const_cast<char*>(message));
    }

	void PlatformLoggerFileError(const char* message, uint8_t color)
    {
        PlatformLoggerFileWrite(message, color);
    } 
}

#endif