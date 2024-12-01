// Temporary, probably need to turn this into platform specific code in the future
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "filesystem.h"
#include "Core/blitmemory.h"

namespace BlitzenPlatform
{
    uint8_t FilepathExists(const char* path)
    {
        struct stat buffer;
        return stat(path, &buffer) == 0;
    }

    uint8_t OpenFile(const char* path, FileModes mode, uint8_t binary, FileHandle& outHandle)
    {
        outHandle.isValid = 0;
        outHandle.pHandle = nullptr;
        const char* modeStr;

        // Try to determine the file mode and turn it into a string
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
        // If what was passed makes no sense to the system, write an error message and exit 
        else 
        {
            BLIT_ERROR("Invalid mode passed while trying to open file: '%s'", path);
            return 0;
        }

        // If the previous check is done tries to read the file
        FILE* file = fopen(path, modeStr);
        // If the file cannot be read, write an error message and exit
        if (!file) 
        {
            BLIT_ERROR("Error opening file: '%s'", path);
            return 0;
        }

        // If everything goes well update the file handle and exit successfully
        outHandle.pHandle = file;
        outHandle.isValid = 1;
        return 1;
    }

    void CloseFile(FileHandle& handle)
    {
        if (handle.pHandle) 
        {
            fclose(reinterpret_cast<FILE*>(handle.pHandle));
            handle.pHandle = nullptr;
            handle.isValid = 0;
        }
    }

    uint8_t FilesystemReadLine(FileHandle& handle, char** lineBuffer)
    {
        if (handle.pHandle) 
        {
            // Since we are reading a single line, it should be safe to assume this is enough characters.
            char buffer[32000];
            if (fgets(buffer, 32000, reinterpret_cast<FILE*>(handle.pHandle))) 
            {
                uint64_t length = strlen(buffer);
                *lineBuffer = reinterpret_cast<char*>(BlitzenCore::BlitAlloc(BlitzenCore::AllocationType::String, sizeof(char) * length + 1));
                strcpy(*lineBuffer, buffer);
                return 1;
            }
        }
        return 0;
    }

    uint8_t FilesystemWriteLine(FileHandle& handle, const char* text)
    {
        if (handle.pHandle) 
        {
            int32_t result = fputs(text, reinterpret_cast<FILE*>(handle.pHandle));
            if (result != EOF) 
            {
                result = fputc('\n', reinterpret_cast<FILE*>(handle.pHandle));
            }
            // Make sure to flush the stream so it is written to the file immediately.
            // This prevents data loss in the event of a crash.
            fflush(reinterpret_cast<FILE*>(handle.pHandle));
            return result != EOF;
        }
        return 0;
    }

    uint8_t FilesystemRead(FileHandle& handle, size_t size, void* pDataRead, size_t* bytesRead)
    {
        if (handle.pHandle && pDataRead) 
        {
            *bytesRead = fread(pDataRead, 1, size, reinterpret_cast<FILE*>(handle.pHandle));
            if (*bytesRead != size) 
            {
                return 0;
            }
            return 1;
        }
        return 0;
    }

    uint8_t FilesystemWrite(FileHandle& handle, size_t size, const void* pData, size_t* bitesWritten)
    {
        if (handle.pHandle) 
        {
            *bitesWritten = fwrite(pData, 1, size, reinterpret_cast<FILE*>(handle.pHandle));
            if (*bitesWritten != size) 
            {
                return 0;
            }
            fflush(reinterpret_cast<FILE*>(handle.pHandle));
            return 1;
        }
        return 0;
    }

    uint8_t FilesystemReadAllBytes(FileHandle& handle, uint8_t** pBytesRead, size_t* byteCount)
    {
        if (handle.pHandle) 
        {
            // File size
            fseek(reinterpret_cast<FILE*>(handle.pHandle), 0, SEEK_END);
            uint64_t size = ftell(reinterpret_cast<FILE*>(handle.pHandle));
            rewind(reinterpret_cast<FILE*>(handle.pHandle));
            *pBytesRead = reinterpret_cast<uint8_t*>(BlitzenCore::BlitAlloc(BlitzenCore::AllocationType::String, sizeof(char) * size));
            *byteCount = fread(*pBytesRead, 1, size, reinterpret_cast<FILE*>(handle.pHandle));
            if (*byteCount != size) 
            {
                return 0;
            }
            return 1;
        }
        return 0;
    }
}