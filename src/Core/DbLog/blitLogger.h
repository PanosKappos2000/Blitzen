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
    void PlatformConsoleWrite(const char* message, BlitzenCore::LogLevel level);
    void PlatformConsoleError(const char* message, BlitzenCore::LogLevel level);

    void PlatformLoggerFileWrite(const char* message, BlitzenCore::LogLevel color);
	void PlatformLoggerFileError(const char* message, BlitzenCore::LogLevel color);
}

namespace BlitzenCore
{

    constexpr const char* GetLoggerLevelStyle(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::FATAL: return "{FATAL}: ";
        case LogLevel::ERR: return "{ERROR}: ";
        case LogLevel::INFO: return "{Info}: ";
        case LogLevel::WARN: return "{Warning}: ";
        case LogLevel::DEBUG: return "{Debug}: ";
        case LogLevel::TRACE: return "{Trace}: ";
        case LogLevel::SUCCESS: default: return "{UNKNOWN_LOG}: ";
        }
    }

    bool InitLogging();
    
    template<typename... ARGS>
    void BlitLog(LogLevel level, const char* msg, ARGS... args)
    {
#if defined(NDEBUG)

        if (!BlitzenCore::BLIT_CHECK_FAIL((int64_t)level))
        {
            return;
        }

#endif
        char outMessage[CE_MESSAGE_BUFFER_SIZE]{""};
        snprintf(outMessage, CE_MESSAGE_BUFFER_SIZE, msg, std::forward<ARGS>(args)...);

        char outMessage2[CE_MESSAGE_BUFFER_SIZE]{""};
        snprintf(outMessage2, CE_MESSAGE_BUFFER_SIZE,"%s%s\n", GetLoggerLevelStyle(level), outMessage);

#if !defined(BLIT_CONSOLE_LOGGER)

		if (BlitzenCore::BLIT_CHECK_FAIL(level))
		{
			BlitzenPlatform::PlatformLoggerFileError(outMessage2, level);
		}
		else
		{
			BlitzenPlatform::PlatformLoggerFileWrite(outMessage2, level);
		}

#else
        
        if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)level))
        {
            BlitzenPlatform::PlatformConsoleError(outMessage2, level);
        }
        else 
        {
            BlitzenPlatform::PlatformConsoleWrite(outMessage2, level);
        }

#endif
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
#define BLIT_ERROR(message, ...)     BlitzenCore::BlitLog(BlitzenCore::LogLevel::ERR, message, ##__VA_ARGS__);
#else
#define BLIT_ERROR(message, ...)    ;
#endif

#if defined(LOGGER_LEVEL_INFO)
template<typename... ARGS>
constexpr void BLIT_INFO(const char* message, ARGS... args)
{
    BlitzenCore::BlitLog(BlitzenCore::LogLevel::INFO, message, std::forward<ARGS>(args)...);
}
#else
#define BLIT_INFO(message, ...)      ;
#endif

#if defined(LOGGER_LEVEL_WARN)
#define BLIT_WARN(message, ...)    BlitLog(BlitzenCore::LogLevel::WARN, message, ##__VA_ARGS__);
#else
#define BLIT_WARN(message, ...)     ;
#endif

#if defined(LOGGER_LEVEL_DEBUG)
template<typename... ARGS>
constexpr void BLIT_DBLOG(const char* message, ARGS... args)
{
    BlitzenCore::BlitLog(BlitzenCore::LogLevel::DEBUG, message, std::forward<ARGS>(args)...);
}
#else
#define BLIT_DBLOG(message, ...)    ;
#endif

#if defined(LOGGER_LEVEL_TRACE)
template<typename... ARGS>
constexpr void BLIT_TRACE(const char* message, ARGS... args)
{
    BlitzenCore::BlitLog(BlitzenCore::LogLevel::TRACE, message, std::forward<ARGS>(args)...);
}
#else
#define BLIT_TRACE(message, ...)    ;
#endif