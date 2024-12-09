#include "blitLogger.h"
#include "blitAssert.h"
#include "Platform/platform.h"

#include "mainEngine.h"

#include <stdarg.h>

namespace BlitzenCore
{
    uint8_t InitLogging()
    {
        return 1;
    }

    void ShutdownLogging()
    {
        if(BlitzenEngine::Engine::GetEngineInstancePointer()->GetEngineSystems().logSystem)
        {
            BLIT_ERROR("Blitzen has not given permission for logging to shutdown")
            return;
        }
    }

    void Log(LogLevel level, const char* message, ...)
    {
        const char* logLevels[6] = {"{FATAL}: ", "{ERROR}: ", "{Info}: ", "{Warning}: ", "{Debug}: ", "{Trace}: "};
        uint8_t isError = level < LogLevel::INFO;

        //Temporary to avoid dynamic arrays for now
        char outMessage[1500];
        memset(outMessage, 0, sizeof(outMessage));

        #if _MSC_VER
            va_list argPtr;
        #else
            __builtin_va_list argPtr;
        #endif
        
        va_start(argPtr, message);
        vsnprintf(outMessage, 1500, message, argPtr);
        va_end(argPtr);

        char outMessage2[1500];
        sprintf(outMessage2, "%s%s\n", logLevels[static_cast<uint8_t>(level)], outMessage);

        // The way error messages are handled varies from platform to platform
        if (isError) 
        {
            BlitzenPlatform::PlatformConsoleError(outMessage2, static_cast<uint8_t>(level));
        } 
        else 
        {
            BlitzenPlatform::PlatformConsoleWrite(outMessage2, static_cast<uint8_t>(level));
        }
    }


    void ReportAssertionFailure(const char* expression, const char* message, const char* file, int32_t line)
    {
        Log(LogLevel::FATAL, "Assertion failure: %s, message: %s, in file: %s, line: %d", expression, message, file, line);
    }
}