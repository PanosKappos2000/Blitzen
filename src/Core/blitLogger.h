#pragma once

#include <stdio.h>
#include <cstdint>
#include <cstddef>

namespace BlitzenCore
{
    #define LOGGER_LEVEL_FATAL      1
    #define LOGGER_LEVEL_ERROR      1

    #if !defined(NDEBUG)
        #define LOGGER_LEVEL_INFO       1
        #define LOGGER_LEVEL_WARN       1
        #define LOGGER_LEVEL_DEBUG      1
        #define LOGGER_LEVEL_TRACE      1
    #else
        #define LOGGER_LEVEL_INFO       0
        #define LOGGER_LEVEL_WARN       0
        #define LOGGER_LEVEL_DEBUG      0
        #define LOGGER_LEVEL_TRACE      0
    #endif

    enum class LogLevel : uint8_t
    {
        FATAL = 0,
        Error = 1,
        Info = 2,
        Warn = 3,
        Debug = 4,
        Trace = 5,
    };

    uint8_t InitLogging();
    void ShutdownLogging();
    void Log(LogLevel level, const char* message, ...);

    #if LOGGER_LEVEL_FATAL
        #define BLIT_FATAL(message, ...)     Log(BlitzenCore::LogLevel::FATAL, message, ##__VA_ARGS__);
    #else
        #define BLIT_FATAL(message, ...)    ;
    #endif

    #if LOGGER_LEVEL_ERROR
        #define BLIT_ERROR(message, ...)     Log(BlitzenCore::LogLevel::Error, message, ##__VA_ARGS__);
    #else
        #define BLIT_ERROR(message, ...)    ;
    #endif

    #if LOGGER_LEVEL_INFO
        #define BLIT_INFO(message, ...)      BlitLog(BlitzenCore::LogLevel::Info, message, ##__VA_ARGS__);
    #else
        #define BLIT_INFO(message, ...)      ;
    #endif

    #if LOGGER_LEVEL_WARN
        #define BLIT_WARN(message, ...)     Log(BlitzenCore::LogLevel::Warn, message, ##__VA_ARGS__);
    #else
        #define BLIT_WARN(message, ...)     ;
    #endif

    #if LOGGER_LEVEL_DEBUG
        #define BLIT_DBLOG(message, ...)     Log(BlitzenCore::LogLevel::Debug, message, ##__VA_ARGS__);
    #else
        #define BLIT_DBLOG(message, ...)    ;
    #endif

    #if LOGGER_LEVEL_TRACE
        #define BLIT_TRACE(message, ...)     Log(BlitzenCore::LogLevel::Trace, message, ##__VA_ARGS__);
    #else
        #define BLIT_TRACE(message, ...)    ;
    #endif

    template<typename... Args>
    void BlitLog(LogLevel level, const char* msg, Args... args)
    {
        constexpr const char* levels[6] =
        {
            "{FATAL}: ",
            "{ERROR}: ",
            "{Info}: ",
            "{Warning}: ",
            "{Debug}: ",
            "{Trace}: "
        };

        uint8_t isError = level < LogLevel::Info;

        char outMessage[1500];
        memset(outMessage, 0, sizeof(outMessage));
        snprintf(outMessage, 1500, msg, args...);

        char outMessage2[1500];
        snprintf(outMessage2, 1500,"%s%s\n", levels[static_cast<uint8_t>(level)], outMessage);

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
}