#include "blitLogger.h"
#include "blitAssert.h"
#include "Platform/platform.h"
#include "Core/blitzenEngine.h"
#include <stdarg.h>
#include <cstring>

namespace BlitzenCore
{
    bool InitLogging()
    {
        BLIT_INFO("%s Booting", BlitzenCore::Ce_BlitzenVersion);
        return true;
    }

    void ShutdownLogging(size_t totalAllocated, size_t* typeAllocations)
    {
        // Warn the user of any memory leaks to look for
        if (totalAllocated)
        {
            #if defined(BLIT_REIN_SANT_ENG)
            BLIT_WARN("There is still unfreed memory, Total: %i \n Unfreed Dynamic Array memory: %i \n Unfreed Hashmap memory: %i \n Unfreed Queue memory: %i \n Unfreed BST memory: %i \n Unfreed String memory: %i \n Unfreed Engine memory: %i \n Unfreed Renderer memory: %i",
                totalAllocated, typeAllocations[0], typeAllocations[1], typeAllocations[2], typeAllocations[3], typeAllocations[4], typeAllocations[5], typeAllocations[6]);
            #else
            printf("There is still unfreed memory, Total: %i \n Unfreed Dynamic Array memory: %i \n Unfreed Hashmap memory: %i \n Unfreed Queue memory: %i \n Unfreed BST memory: %i \n Unfreed String memory: %i \n Unfreed Engine memory: %i \n Unfreed Renderer memory: %i",
                totalAllocated, typeAllocations[0], typeAllocations[1], typeAllocations[2], typeAllocations[3], typeAllocations[4], typeAllocations[5], typeAllocations[6]);
            #endif
        }
    }

    // Used by assertions to print to the window
    void ReportAssertionFailure(const char* expression, const char* message, const char* file, int32_t line)
    {
        BlitLog(LogLevel::FATAL, 
            "Assertion failure: %s, message: %s, in file: %s, line: %d", 
            expression, message, file, line);
    }
}