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
        BLIT_INFO("%s Booting", BlitzenEngine::ce_blitzenVersion);
        return true;
    }

    void ShutdownLogging()
    {
        
    }

    // Used by assertions to print to the window
    void ReportAssertionFailure(const char* expression, const char* message, const char* file, int32_t line)
    {
        BlitLog(LogLevel::FATAL, 
            "Assertion failure: %s, message: %s, in file: %s, line: %d", 
            expression, message, file, line);
    }
}