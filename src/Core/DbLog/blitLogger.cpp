#include "blitLogger.h"
#include "blitAssert.h"
#include <stdarg.h>
#include <cstring>

namespace BlitzenCore
{
    bool InitLogging()
    {
        BlitLog(BlitzenCore::LogLevel::Info, "%s Booting", BlitzenCore::Ce_BlitzenVersion);
        return true;
    }

    void ShutdownLogging(size_t totalAllocated, size_t* typeAllocations)
    {
        // Warn the user of any memory leaks to look for
        if (totalAllocated)
        {
            #if defined(BLIT_REIN_SANT_ENG)
            BlitLog(BlitzenCore::LogLevel::Warn, 
                "There is still unfreed memory--\n \
                Total: %i \n \
                Unfreed Dynamic Array memory: %i \n \
                Unfreed Hashmap memory: %i \n \
                Unfreed Queue memory: %i \n \
                Unfreed BST memory: %i \n \
                Unfreed String memory: %i \n \
                Unfreed Engine memory: %i \n \
                Unfreed Renderer memory: %i \n \
                Unfreed Entity memory: %i \n \
                Unfreed Entity node memory: %i \n \
                Unfreed Scene memory: %i \n \
                Unfreed SmartPtr memory: %i \n",
                totalAllocated, typeAllocations[0], typeAllocations[1], typeAllocations[2], typeAllocations[3], typeAllocations[4], typeAllocations[5], typeAllocations[6], typeAllocations[7], 
                typeAllocations[8], typeAllocations[9], typeAllocations[10]);
            #else
            printf("There is still unfreed memory--\n \
                Total: %i \n \
                Unfreed Dynamic Array memory: %i \n \
                Unfreed Hashmap memory: %i \n \
                Unfreed Queue memory: %i \n \
                Unfreed BST memory: %i \n \
                Unfreed String memory: %i \n \
                Unfreed Engine memory: %i \n \
                Unfreed Renderer memory: %i \n \
                Unfreed Entity memory: %i \n \
                Unfreed Entity node memory: %i \n \
                Unfreed Scene memory: %i \n \
                Unfreed SmartPtr memory: %i \n",
                totalAllocated, typeAllocations[0], typeAllocations[1], typeAllocations[2], typeAllocations[3], typeAllocations[4], typeAllocations[5], typeAllocations[6], typeAllocations[7],
                typeAllocations[8], typeAllocations[9], typeAllocations[10]);
            #endif
        }
    }

    void ReportAssertionFailure(const char* expression, const char* message, const char* file, int32_t line)
    {
        BlitLog(LogLevel::FATAL, "Assertion failure: %s, message: %s, in file: %s, line: %d", expression, message, file, line);
    }
}