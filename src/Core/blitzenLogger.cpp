#include "blitLogger.h"
#include "blitAssert.h"
#include "Platform/platform.h"
#include "Engine/blitzenEngine.h"
#include <stdarg.h>
#include <cstring>

namespace BlitzenCore
{
    bool InitLogging()
    {
        BLIT_INFO("%s Booting", BlitzenEngine::ce_blitzenVersion)
        return true;
    }

    void ShutdownLogging()
    {
        
    }

    void Log(LogLevel level, const char* message, ...)
    {
        uint8_t isError = level < LogLevel::Info;

        // Temporary to avoid dynamic arrays for now, even though this is terrible
        char outMessage[1500];
        memset(outMessage, 0, sizeof(outMessage));

        #if defined(_WIN32)
            va_list argPtr;
        #else
            __builtin_va_list argPtr;
        #endif
        
        va_start(argPtr, message);
        vsnprintf(outMessage, 1500, message, argPtr);
        va_end(argPtr);

        char outMessage2[1500];
        snprintf(outMessage2, 1500, "%s%s\n", logLevels[static_cast<uint8_t>(level)], outMessage);

        // Creates a custom error message
        if (isError) 
        {
            BlitzenPlatform::PlatformConsoleError(outMessage2, static_cast<uint8_t>(level));
        }

        // Creates a custom non error message 
        else 
        {
            BlitzenPlatform::PlatformConsoleWrite(outMessage2, static_cast<uint8_t>(level));
        }
    }

    // Used by assertions to print to the window
    void ReportAssertionFailure(const char* expression, const char* message, const char* file, int32_t line)
    {
        Log(LogLevel::FATAL, "Assertion failure: %s, message: %s, in file: %s, line: %d", expression, message, file, line);
    }
}