#pragma once

#include <iostream>

namespace BlitzenCore
{
    #define LOGGER_LEVEL_FATAL      1
    #define LOGGER_LEVEL_ERROR      1

    #ifndef NDEBUG
        #define LOGGER_LEVEL_INFO       1
        #define LOGGER_LEVEL_WARN       1
        #define LOGGER_LEVEL_DEBUG      1
        #define LOGGER_LEVEL_TRACE      1
    #else
        #define LOGGER_LEVEL_WARN       0
        #define LOGGER_LEVEL_DEBUG      0
        #define LOGGER_LEVEL_TRACE      0
    #endif

    enum class LogLevel : uint8_t
    {
        FATAL = 0,
        ERROR = 1,
        INFO = 2,
        WARN = 3,
        DEBUG = 4,
        TRACE = 5,
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
        #define BLIT_ERROR(message, ...)     Log(BlitzenCore::LogLevel::ERROR, message, ##__VA_ARGS__);
    #else
        #define BLIT_ERROR(message, ...)    ;
    #endif

    #if LOGGER_LEVEL_INFO
        #define BLIT_INFO(message, ...)      Log(BlitzenCore::LogLevel::INFO, message, ##__VA_ARGS__);
    #else
        #define BLIT_INFO(message, ...)      ;
    #endif

    #if LOGGER_LEVEL_WARN
        #define BLIT_WARN(message, ...)     Log(BlitzenCore::LogLevel::WARN, message, ##__VA_ARGS__);
    #else
        #define BLIT_WARN(message, ...)     ;
    #endif

    #if LOGGER_LEVEL_DEBUG
        #define BLIT_DBLOG(message, ...)     Log(BlitzenCore::LogLevel::DEBUG, message, __VA_ARGS__);
    #else
        #define BLIT_DBLOG(message, ...)    ;
    #endif

    #if LOGGER_LEVEL_TRACE
        #define BLIT_TRACE(message, ...)     Log(BlitzenCore::LogLevel::TRACE, message, __VA_ARGS__);
    #else
        #define BLIT_TRACE(message, ...)    ;
    #endif
}