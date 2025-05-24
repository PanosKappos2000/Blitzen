#pragma once
#include "Core/blitzenEngine.h"
#include <utility>

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
    
    template<typename... ARGS>
    void BlitLog(LogLevel level, const char* msg, ARGS... args)
    {
        char outMessage[CE_MESSAGE_BUFFER_SIZE]{""};
        snprintf(outMessage, CE_MESSAGE_BUFFER_SIZE, msg, std::forward<ARGS>(args)...);

        char outMessage2[CE_MESSAGE_BUFFER_SIZE]{""};
        snprintf(outMessage2, CE_MESSAGE_BUFFER_SIZE,"%s%s\n", CE_LOGGER_LEVELS[uint8_t(level)], outMessage);

        bool isError = level < LogLevel::Info;
        if (isError) 
        {
            BlitzenPlatform::PlatformConsoleError(outMessage2, uint8_t(level));
        }
        else 
        {
            BlitzenPlatform::PlatformConsoleWrite(outMessage2, uint8_t(level));
        }
    }
}

// Automatic constexpr functions for used in place of BlitLog
// Preferable because most of them are deactivated on release configuration
#if defined(LOGGER_LEVEL_FATAL)
template<typename... ARGS>
constexpr void BLIT_FATAL(const char* message, ARGS... args)
{
    BlitzenCore::BlitLog(BlitzenCore::LogLevel::FATAL, message, std::forward< ARGS>(args)...);
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
template<typename... ARGS>
constexpr void BLIT_INFO(const char* message, ARGS... args)
{
    BlitzenCore::BlitLog(BlitzenCore::LogLevel::Info, message, std::forward<ARGS>(args)...);
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
template<typename... ARGS>
constexpr void BLIT_DBLOG(const char* message, ARGS... args)
{
    BlitzenCore::BlitLog(BlitzenCore::LogLevel::Debug, message, std::forward<ARGS>(args)...);
}
#else
#define BLIT_DBLOG(message, ...)    ;
#endif

#if defined(LOGGER_LEVEL_TRACE)
template<typename... ARGS>
constexpr void BLIT_TRACE(const char* message, ARGS... args)
{
    BlitzenCore::BlitLog(BlitzenCore::LogLevel::Trace, message, std::forward<ARGS>(args)...);
}
#else
#define BLIT_TRACE(message, ...)    ;
#endif