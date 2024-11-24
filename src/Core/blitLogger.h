#pragma once

#include <iostream>

namespace BlitzenCore
{
    #define LOGGER_LEVEL_FATAL 1
    #define LOGGER_LEVEL_ERROR 1

    #ifndef NDEBUG
        #define LOGGER_LEVEL_WARN 1
        #define LOGGER_LEVEL_DEBUG 1
        #define LOGGER_LEVEL_TRACE 1
    #else
        #define LOGGER_LEVEL_WARN 0
        #define LOGGER_LEVEL_DEBUG 0
        #define LOGGER_LEVEL_TRACE 0
    #endif

    enum class LogLevel : uint8_t
    {
        FATAL = 0,
        ERROR = 1,
        WARN = 2,
        DEBUG = 3,
        TRACE = 4,
    };

    uint8_t InitLogging();
    void ShutdownLogging();
    void Log(LogLevel level, const char* message, ...);

    #if LOGGER_LEVEL_FATAL
        #define FATAL_MESSAGE(message, ...)     Log(BlitzenCore::LogLevel::FATAL, message, __VA_ARGS__);
    #else
        #define FATAL_MESSAGE(message, ...)
    #endif

    #if LOGGER_LEVEL_ERROR
        #define ERROR_MESSAGE(message, ...)     Log(BlitzenCore::LogLevel::ERROR, message, __VA_ARGS__);
    #else
        #define ERROR_MESSAGE(message, ...)
    #endif

    #if LOGGER_LEVEL_WARN
        #define WARNING_MESSAGE(message, ...)     Log(BlitzenCore::LogLevel::WARN, message, __VA_ARGS__);
    #else
        #define WARNING_MESSAGE(message, ...)
    #endif

    #if LOGGER_LEVEL_DEBUG
        #define DEBUG_MESSAGE(message, ...)     Log(BlitzenCore::LogLevel::DEBUG, message, __VA_ARGS__);
    #else
        #define DEBUG_MESSAGE(message, ...)
    #endif

    #if LOGGER_LEVEL_TRACE
        #define TRACE_MESSAGE(message, ...)     Log(BlitzenCore::LogLevel::TRACE, message, __VA_ARGS__);
    #else
        #define TRACE_MESSAGE(message, ...)
    #endif
}