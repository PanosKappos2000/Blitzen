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

    constexpr const char* LogLevels[6] =
    {
        "{FATAL}: ",
        "{ERROR}: ",
        "{Info}: ",
        "{Warning}: ",
        "{Debug}: ",
        "{Trace}: "
    };

    constexpr uint32_t MessageBufferSize = 1500;
    
    template<typename... Args>
    void BlitLog(LogLevel level, const char* msg, Args... args)
    {
        char outMessage[MessageBufferSize];
        snprintf(outMessage, MessageBufferSize, msg, args...);
        char outMessage2[MessageBufferSize];
        snprintf(outMessage2, MessageBufferSize,"%s%s\n", LogLevels[static_cast<uint8_t>(level)], outMessage);

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

// Automatic constexpr functions for used in place of BlitLog
// Preferable because most of them are deactivated on release configuration
#if defined(LOGGER_LEVEL_FATAL)
template<typename... Args>
constexpr void BLIT_FATAL(const char* message, Args... args)
{
    BlitzenCore::BlitLog(BlitzenCore::LogLevel::FATAL, message, args...);
}
#else
#define BLIT_FATAL(message, ...)    ;
#endif

#if defined(LOGGER_LEVEL_ERROR)
#define BLIT_ERROR(message, ...)     BlitLog(BlitzenCore::LogLevel::Error, message, ##__VA_ARGS__);
#else
#define BLIT_ERROR(message, ...)    ;
#endif

#if defined(LOGGER_LEVEL_INFO)
template<typename... Args>
constexpr void BLIT_INFO(const char* message, Args... args)
{
    BlitzenCore::BlitLog(BlitzenCore::LogLevel::Info, message, args...);
}
#else
#define BLIT_INFO(message, ...)      ;
#endif

#if defined(LOGGER_LEVEL_WARN)
#define BLIT_WARN(message, ...)    BlitLog(BlitzenCore::LogLevel::Warn, message, ##__VA_ARGS__);
#else
#define BLIT_WARN(message, ...)     ;
#endif

#if defined(LOGGER_LEVEL_DEBUG)
template<typename... Args>
constexpr void BLIT_DBLOG(const char* message, Args... args)
{
    BlitzenCore::BlitLog(BlitzenCore::LogLevel::Debug, message, args...);
}
#else
#define BLIT_DBLOG(message, ...)    ;
#endif

#if defined(LOGGER_LEVEL_TRACE)
template<typename... Args>
constexpr void BLIT_TRACE(const char* message, Args... args)
{
    BlitzenCore::BlitLog(BlitzenCore::LogLevel::Trace, message, args...);
}
#else
#define BLIT_TRACE(message, ...)    ;
#endif