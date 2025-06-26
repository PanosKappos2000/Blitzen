#include "blitLogger.h"
#include "blitAssert.h"
#include <stdarg.h>
#include <cstring>

namespace BlitzenCore
{
    bool InitLogging()
    {
        BlitLog(BlitzenCore::LogLevel::INFO, "%s Booting", BlitzenCore::CE_BLITZEN);
        return true;
    }

    bool LOG_ERROR_MSG_AND_RETURN(const char* system, const char* msg)
    {
        BLIT_ERROR("%s::MSG: %s", system, msg);
        return false;
    }

    bool BLIT_CHECK_SUCCESS(int64_t code)
    {
        return code == CE_BLITZEN_SUCCESS;
    }

    bool BLIT_CHECK_FAIL(int64_t code)
    {
        return code < CE_BLITZEN_SUCCESS;
    }

    bool BLIT_CHECK_FATAL(int64_t code)
    {
        return code < CE_BLITZEN_FATAL;
    }

    void LogAllocation(AllocationType alloc, size_t size, AllocationAction action)
    {
        static size_t totalAllocated{ 0 };
        static size_t typeAllocations[size_t(AllocationType::MaxTypes)]{ 0 };

        if (action == AllocationAction::ALLOC)
        {
            totalAllocated += size;
            typeAllocations[uint8_t(alloc)] += size;
        }
        else if (action == AllocationAction::FREE)
        {
            totalAllocated -= size;
            typeAllocations[uint8_t(alloc)] -= size;
        }
        else if (action == AllocationAction::FREE_ALL)
        {
            ShutdownLogging(totalAllocated, typeAllocations);
        }
    }

    Engine::~Engine()
    {
        LogAllocation(AllocationType::Engine, 0, AllocationAction::FREE_ALL);
    }

    void ShutdownLogging(size_t totalAllocated, size_t* typeAllocations)
    {
        // Warn the user of any memory leaks to look for
        if (totalAllocated)
        {
            #if defined(BLIT_REIN_SANT_ENG)
            BlitLog(BlitzenCore::LogLevel::WARN, 
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
                Unfreed SmartPtr memory: %i \n, \
                Unfreed Linear Allocator memory: %i \n \
                Unfreed World Variable memory: %i \n \
                Unfreed Triangle Memory: %i \n", 
                totalAllocated, typeAllocations[0], typeAllocations[1], typeAllocations[2], typeAllocations[3], typeAllocations[4], typeAllocations[5], typeAllocations[6], typeAllocations[7], 
                typeAllocations[8], typeAllocations[9], typeAllocations[10], typeAllocations[11], typeAllocations[12], typeAllocations[13]);
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