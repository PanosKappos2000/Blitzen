#pragma once
#include <stdio.h>
#include <cstdint>
#include <cstddef>

#define LOGGER_LEVEL_FATAL
#define LOGGER_LEVEL_ERROR
#if !defined(NDEBUG)
    #define LOGGER_LEVEL_INFO 
    #define LOGGER_LEVEL_WARN
    #define LOGGER_LEVEL_DEBUG
    #define LOGGER_LEVEL_TRACE
#endif

namespace BlitzenPlatform
{
    void PlatformConsoleWrite(const char* message, uint8_t color);
    void PlatformConsoleError(const char* message, uint8_t color);
}

namespace BlitzenCore
{
    enum class LogLevel : uint8_t
    {
        FATAL = 0,
        Error = 1,
        Info = 2,
        Warn = 3,
        Debug = 4,
        Trace = 5,
    };

    bool InitLogging();
    void ShutdownLogging();
    void Log(LogLevel level, const char* message, ...);

    #if defined(LOGGER_LEVEL_FATAL)
        #define BLIT_FATAL(message, ...)     BlitLog(BlitzenCore::LogLevel::FATAL, message, ##__VA_ARGS__);
    #else
        #define BLIT_FATAL(message, ...)    ;
    #endif

    #if defined(LOGGER_LEVEL_ERROR)
        #define BLIT_ERROR(message, ...)     BlitLog(BlitzenCore::LogLevel::Error, message, ##__VA_ARGS__);
    #else
        #define BLIT_ERROR(message, ...)    ;
    #endif

    #if defined(LOGGER_LEVEL_INFO)
        #define BLIT_INFO(message, ...)      BlitLog(BlitzenCore::LogLevel::Info, message, ##__VA_ARGS__);
    #else
        #define BLIT_INFO(message, ...)      ;
    #endif

    #if defined(LOGGER_LEVEL_WARN)
        #define BLIT_WARN(message, ...)      BlitLog(BlitzenCore::LogLevel::Warn, message, ##__VA_ARGS__);
    #else
        #define BLIT_WARN(message, ...)     ;
    #endif

    #if defined(LOGGER_LEVEL_DEBUG)
        #define BLIT_DBLOG(message, ...)     BlitLog(BlitzenCore::LogLevel::Debug, message, ##__VA_ARGS__);
    #else
        #define BLIT_DBLOG(message, ...)    ;
    #endif

    #if defined(LOGGER_LEVEL_TRACE)
        #define BLIT_TRACE(message, ...)     BlitLog(BlitzenCore::LogLevel::Trace, message, ##__VA_ARGS__);
    #else
        #define BLIT_TRACE(message, ...)    ;
    #endif

    constexpr const char* logLevels[6] =
    {
        "{FATAL}: ",
        "{ERROR}: ",
        "{Info}: ",
        "{Warning}: ",
        "{Debug}: ",
        "{Trace}: "
    };
    
    template<typename... Args>
    void BlitLog(LogLevel level, const char* msg, Args... args)
    {
        char outMessage[1500];
        snprintf(outMessage, 1500, msg, args...);
        char outMessage2[1500];
        snprintf(outMessage2, 1500,"%s%s\n", logLevels[static_cast<uint8_t>(level)], outMessage);

        bool isError = level < LogLevel::Info;
        if (isError) 
        {
            BlitzenPlatform::PlatformConsoleError(outMessage2, static_cast<uint8_t>(level));
        }
        else 
        {
            BlitzenPlatform::PlatformConsoleWrite(outMessage2, static_cast<uint8_t>(level));
        }
    }
}