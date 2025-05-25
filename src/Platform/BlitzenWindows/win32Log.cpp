#if defined(_WIN32)

#include "Platform/blitPlatformContext.h"
#include "Platform/Common/blitMappedFile.h"

namespace BlitzenPlatform
{
    static void BlitWinLogSetup(const char* message, uint8_t color, HANDLE consoleHandle)
    {
        SetConsoleTextAttribute(consoleHandle, BlitzenCore::CE_PLATFORM_CONSOLE_LOGGER_COLORS[color]);

        OutputDebugStringA(message);
        uint64_t length = strlen(message);
        LPDWORD numberWritten = 0;

        WriteConsoleA(consoleHandle, message, static_cast<DWORD>(length), numberWritten, 0);
    }

    void PlatformConsoleWrite(const char* message, uint8_t color)
    {
        auto consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        BlitWinLogSetup(message, color, consoleHandle);
    }

    void PlatformConsoleError(const char* message, uint8_t color)
    {
        auto consoleHandle = GetStdHandle(STD_ERROR_HANDLE);
        BlitWinLogSetup(message, color, consoleHandle);
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