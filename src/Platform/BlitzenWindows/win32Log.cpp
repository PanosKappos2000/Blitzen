#if defined(_WIN32)

#include "Platform/blitPlatformContext.h"
#include "Platform/Common/blitMappedFile.h"

namespace BlitzenPlatform
{
    static constexpr uint8_t GetLoggerColors(BlitzenCore::LogLevel level)
    {
        switch (level)
        {
        case BlitzenCore::LogLevel::FATAL: return 64;
        case BlitzenCore::LogLevel::ERR: return 4;
        case BlitzenCore::LogLevel::INFO: return 6;
        case BlitzenCore::LogLevel::WARN: return 2;
        case BlitzenCore::LogLevel::DEBUG: return 1;
        case BlitzenCore::LogLevel::TRACE: return 8;
        case BlitzenCore::LogLevel::SUCCESS: default: return 4;
        }
    }

    static void BlitWinLogSetup(const char* message, BlitzenCore::LogLevel level, HANDLE consoleHandle)
    {
        SetConsoleTextAttribute(consoleHandle, GetLoggerColors(level));

        OutputDebugStringA(message);
        uint64_t length = strlen(message);
        LPDWORD numberWritten = 0;

        WriteConsoleA(consoleHandle, message, static_cast<DWORD>(length), numberWritten, 0);
    }

    void PlatformConsoleWrite(const char* message, BlitzenCore::LogLevel level)
    {
        auto consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        BlitWinLogSetup(message, level, consoleHandle);
    }

    void PlatformConsoleError(const char* message, BlitzenCore::LogLevel level)
    {
        auto consoleHandle = GetStdHandle(STD_ERROR_HANDLE);
        BlitWinLogSetup(message, level, consoleHandle);
    }

    void PlatformLoggerFileWrite(const char* message, BlitzenCore::LogLevel level)
    {
        static MEMORY_MAPPED_FILE_SCOPE s_scopedFile;

        if (s_scopedFile.Failed())
        {
            auto mmfResult{ s_scopedFile.OpenWrite("blitLogOutput.txt", BlitzenCore::Ce_BlitLogOutputFileSize) };
            if (mmfResult != BLIT_MMF_RES::SUCCESS && mmfResult != BLIT_MMF_RES::SUCCESS_FALLBACK)
            {
                const char* mmfErrorString{ GET_BLIT_MMF_RES_ERROR_STR(mmfResult) };
                PlatformConsoleError(mmfErrorString, BlitzenCore::LogLevel::ERR);
                PlatformConsoleWrite(message, level);
                return;
            }
        }

        WriteMemoryMappedFile(s_scopedFile, s_scopedFile.m_endOffset, strlen(message), const_cast<char*>(message));

    }

    void PlatformLoggerFileError(const char* message, BlitzenCore::LogLevel level)
    {
        PlatformLoggerFileWrite(message, level);
    }
}

#endif