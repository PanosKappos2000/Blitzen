// Temporary, probably need to turn this into platform specific code in the future
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "Core/blitLogger.h"
#include "Core/blitAssert.h"
#include "filesystem.h"
#include "Core/blitMemory.h"

namespace BlitzenPlatform
{
    bool FilepathExists(const char* path)
    {
        #if defined(_WIN32)
            struct _stat buffer;
            return _stat(path, &buffer) == 0;
        #else
            struct stat buffer;
            return stat(path, &buffer) == 0;
        #endif
    }

    bool C_FILE_SCOPE::Open(const char* path, FileModes mode, uint8_t binary)
    {
        if (m_pHandle != nullptr)
        {
            BLIT_ERROR("File handle already in use");
            return false;
        }

        const char* modeStr{""};

        // FILE MODE
        if (((uint8_t)mode & (uint8_t)FileModes::Read) != 0 && ((uint8_t)mode & (uint8_t)FileModes::Write) != 0)
        {
            modeStr = binary ? "w+b" : "w+";
        }
        else if (((uint8_t)mode & (uint8_t)FileModes::Read) != 0 && ((uint8_t)mode & (uint8_t)FileModes::Write) == 0)
        {
            modeStr = binary ? "rb" : "r";
        }
        else if (((uint8_t)mode & (uint8_t)FileModes::Read) == 0 && ((uint8_t)mode & (uint8_t)FileModes::Write) != 0)
        {
            modeStr = binary ? "wb" : "w";
        } 
        else
        {
            BLIT_ERROR("Invalid mode passed while trying to open file: '%s'", path);
            return false;
        }

        FILE* file = fopen(path, modeStr);
        if (!file)
        {
            BLIT_ERROR("Error opening file: '%s'", path);
            return false;
        }

        // If everything goes well update the file handle and exit successfully
        m_pHandle = file;
        return true;
    }

    bool C_FILE_SCOPE::Open(const char* path, const char* mode)
    {
        // If the previous check is done tries to read the file
        FILE* file = fopen(path, mode);
        // If the file cannot be read, write an error message and exit
        if (!file)
        {
            BLIT_ERROR("Error opening file: '%s'", path);
            return false;
        }

        // If everything goes well update the file handle and exit successfully
        m_pHandle = file;
        return true;
    }

    C_FILE_SCOPE::~C_FILE_SCOPE()
    {
        Close();
    }

    void C_FILE_SCOPE::Close()
    {
        if (m_pHandle)
        {
            fclose(m_pHandle);
            m_pHandle = nullptr;
        }
    }

    bool FilesystemReadLine(C_FILE_SCOPE& handle, size_t maxLength, char** lineBuffer, size_t* pLength)
    {
        if (handle.m_pHandle && lineBuffer && pLength && maxLength > 0) 
        {
            char* buffer = *lineBuffer;

            if (!buffer)
            {
                BLIT_ERROR("Line Buffer empty");
                return 0;
            }

            if (fgets(buffer, int32_t(maxLength), handle.m_pHandle) != 0) 
            {
                *pLength = strlen(*lineBuffer);
                return 1;
            }
        }
        return 0;
    }

    bool FilesystemWriteLine(C_FILE_SCOPE& handle, const char* text)
    {
        if (handle.m_pHandle) 
        {
            int32_t result = fputs(text, handle.m_pHandle);
            if (result != EOF) 
            {
                result = fputc('\n', handle.m_pHandle);
            }

            // Make sure to flush the stream so it is written to the file immediately.
            // This prevents data loss in the event of a crash.
            fflush(handle.m_pHandle);
            return result != EOF;
        }
        return 0;
    }

    bool FilesystemRead(C_FILE_SCOPE& handle, size_t size, void* pDataRead, size_t* bytesRead)
    {
        if (handle.m_pHandle && pDataRead)
        {
            *bytesRead = fread(pDataRead, 1, size, handle.m_pHandle);

            if (*bytesRead != size) 
            {
                BLIT_ERROR("File read size error");
                return false;
            }

            return true;
        }

        BLIT_ERROR("Null data for file read");
        return false;
    }

    bool FilesystemWrite(C_FILE_SCOPE& handle, size_t size, const void* pData, size_t* bitesWritten)
    {
        if (handle.m_pHandle)
        {
            *bitesWritten = fwrite(pData, 1, size, handle.m_pHandle);
            if (*bitesWritten != size) 
            {
				BLIT_ERROR("File write size error");
                return 0;
            }

            // ALWAYS FLUSH MATE - ANGE POSTECOGLOU
            fflush(handle.m_pHandle);
            return 1;
        }

		BLIT_ERROR("Null data for file write");
        return 0;
    }

    bool FilesystemReadAllBytes(C_FILE_SCOPE& handle, BlitCL::String& str, size_t* byteCount)
    {
        if(handle.m_pHandle)
        {
            // File size
            fseek(handle.m_pHandle, 0, SEEK_END);
            uint64_t size = ftell(handle.m_pHandle);

            rewind(handle.m_pHandle);
            
            str.Resize(size);

            *byteCount = fread(str.Data(), 1, size, handle.m_pHandle);
            if(*byteCount != size)
            {
                BLIT_ERROR("File read size error");
                return 0;
            }

            return 1;
        }

        BLIT_ERROR("Null data for file write");
        return 0;
    }
}