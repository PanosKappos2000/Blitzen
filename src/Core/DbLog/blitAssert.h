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
        void FORCE_ASSERT_CORE_ISSUE(const char* failureDiscovererName, const char* failureOriginatorName, const char* discovererMessage);

        #define BLIT_ASSERT(expr)                                                                               \
                                    if(expr){}                                                                  \
                                    else                                                                        \
                                    {                                                                           \
                                        BlitzenCore::ReportAssertionFailure(#expr, "", __FILE__, __LINE__);     \
                                        BDB_BREAK();                                                               \
                                    }                                                                           \

        #define BLIT_ASSERT_MESSAGE(expr, message)                                                                      \
                                {                                                                                       \
                                    if(expr){}                                                                          \
                                    else                                                                                \
                                    {                                                                                   \
                                        BlitzenCore::ReportAssertionFailure(#expr, message, __FILE__, __LINE__);        \
                                        BDB_BREAK();                                                                       \
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

#if !defined(NDEBUG) && defined(BLIT_ASSERTIONS_ENABLED)
    // CHECKS IF THE EXPRESSION IS TRUE OTHERWISE PERFORMS RUNTIME DEBUG PROTECTION
    #define BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(expr)              if(!(expr))                                                                                              \
                                                                   {                                                                                                        \
                                                                       BlitzenCore::ReportAssertionFailure(#expr, "RUNTIME ERROR", __FILE__, __LINE__);return;              \
                                                                   }
    // CHECKS IF THE EXPRESSION IS TRUE OTHERWISE ASSERTS. 
    // THIS SHOULD BE PREFERRED FOR RUNTIME OVER NORMAL BLIT_ASSERT, AS IT DEACTIVATES ON RELEASE CONFIGURATIONS
    #define BLIT_RUNTIME_TEST_CHECK_ASSERT(expr)                   BLIT_ASSERT((expr));
#else
    #define BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(expr)
    #define BLIT_RUNTIME_TEST_CHECK_ASSERT(expr)
#endif
}