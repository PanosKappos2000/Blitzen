#pragma once
#include <stdio.h>
#include <cstdint>

namespace BlitzenCore
{
    #if defined(BLIT_ASSERTIONS_ENABLED)
        #if defined(_WIN32)
            #define BDB_BREAK() __debugbreak();
        #else
            #define BDB_BREAK() __builtin_trap();
        #endif

        // Implemented in blitzenLogger.cpp
        void ReportAssertionFailure(const char* expression, const char* message, const char* file, int32_t line);

        #define BLIT_ASSERT(expr)                                                                               \
                                    if(expr){}                                                                  \
                                    else                                                                        \
                                    {                                                                           \
                                        BlitzenCore::ReportAssertionFailure(#expr, "", __FILE__, __LINE__);     \
                                        BDB_BREAK()                                                               \
                                    }                                                                           \

        #define BLIT_ASSERT_MESSAGE(expr, message)                                                                      \
                                {                                                                                       \
                                    if(expr){}                                                                          \
                                    else                                                                                \
                                    {                                                                                   \
                                        BlitzenCore::ReportAssertionFailure(#expr, message, __FILE__, __LINE__);        \
                                        BDB_BREAK()                                                                       \
                                    }                                                                                   \
                                }

        #ifndef NDEBUG
            #define BLIT_ASSERT_DEBUG(expr)                                                                     \
                                {                                                                               \
                                    if(expr){}                                                                  \
                                    else                                                                        \
                                    {                                                                           \
                                        BlitzenCore::ReportAssertionFailure(#expr, "", __FILE__, __LINE__);     \
                                        BDB_BREAK()                                                               \
                                    }                                                                           \
                                }
        #else
            #define BLIT_ASSERT_DEBUG(expr)
        #endif
    #else
        #define BLIT_ASSERT(expr)                   expr;
        #define BLIT_ASSERT_MESSAGE(expr, message)  expr;
        #define BLIT_ASSERT_DEBUG(expr)         
    #endif
}