#if defined(linux)

#include "Core/blitzenEngine.h"
#include "Platform/blitPlatformContext.h"
#include <stdio.h>
#include "Platform/Common/blitMappedFile.h"

namespace BlitzenPlatform
{
    static constexpr const char* GetLoggerColor(BlitzenCore::LoggerLevel level)
    {
        switch (level)
        {
        case BlitzenCore::LogLevel::FATAL: return "0;41";
        case BlitzenCore::LogLevel::ERR: return "1;31";
        case BlitzenCore::LogLevel::INFO: return "1;33";
        case BlitzenCore::LogLevel::WARN: return "1;32";
        case BlitzenCore::LogLevel::DEBUG: return "1;34";
        case BlitzenCore::LogLevel::TRACE: return "1;30";
        case BlitzenCore::LogLevel::SUCCESS: default: return "1;31";
        }
            
    }

    void PlatformConsoleWrite(const char* message, BlitzenCore::LoggerLevel level)
    {
        printf("\033[%sm%s\033[0m", GetLoggerColor[level], message);
    }

    void PlatformConsoleError(const char* message, BlitzenCore::LoggerLevel level)
    {
        printf("\033[%sm%s\033[0m", GetLoggerColor[level], message);
    }

    void PlatformLoggerFileWrite(const char* message, BlitzenCore::LoggerLevel level)
    {
        static MEMORY_MAPPED_FILE_SCOPE s_scopedFile;

        if (s_scopedFile.Failed())
        {
            auto mmfResult{ s_scopedFile.OpenWrite("blitLogOutput.txt", BlitzenCore::Ce_BlitLogOutputFileSize) };
            if (mmfResult != BLIT_MMF_RES::SUCCESS && mmfResult != BLIT_MMF_RES::SUCCESS_FALLBACK)
            {
                const char* mmfErrorString{ GET_BLIT_MMF_RES_ERROR_STR(mmfResult) };
                PlatformConsoleError(mmfErrorString, (uint8_t)BlitzenCore::LogLevel::Error);
                PlatformConsoleWrite(message, level);
                return;
            }
        }

        WriteMemoryMappedFile(s_scopedFile, s_scopedFile.m_endOffset, strlen(message), const_cast<char*>(message));
    }

	void PlatformLoggerFileError(const char* message, BlitzenCore::LoggerLevel level)
    {
        PlatformLoggerFileWrite(message, level);
    } 
}

#endif