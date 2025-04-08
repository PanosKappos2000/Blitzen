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
    uint8_t FilepathExists(const char* path)
    {
        #if _MSC_VER
            struct _stat buffer;
            return _stat(path, &buffer) == 0;
        #else
            struct stat buffer;
            return stat(path, &buffer) == 0;
        #endif
    }

    uint8_t FileHandle::Open(const char* path, FileModes mode, uint8_t binary)
    {
        // If the handle already has a valid handle, it asserts
        BLIT_ASSERT(pHandle == nullptr)

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
        pHandle = file;
        return 1;
    }

    uint8_t FileHandle::Open(const char* path, const char* mode)
    {
        // If the previous check is done tries to read the file
        FILE* file = fopen(path, mode);
        // If the file cannot be read, write an error message and exit
        if (!file)
        {
            BLIT_ERROR("Error opening file: '%s'", path);
            return 0;
        }

        // If everything goes well update the file handle and exit successfully
        pHandle = file;
        return 1;
    }

    FileHandle::~FileHandle()
    {
        Close();
    }

    void FileHandle::Close()
    {
        if (pHandle) 
        {
            fclose(reinterpret_cast<FILE*>(pHandle));
            pHandle = nullptr;
        }
    }

    uint8_t FilesystemReadLine(FileHandle& handle, size_t maxLength, char** lineBuffer, size_t* pLength)
    {
        if (handle.pHandle && lineBuffer && pLength && maxLength > 0) 
        {
            char* buffer = *lineBuffer;
            if (!buffer)
                return 0;
            if (fgets(buffer, static_cast<int32_t>(maxLength), (FILE*)handle.pHandle) != 0) 
            {
                *pLength = strlen(*lineBuffer);
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
            *pBytesRead = reinterpret_cast<uint8_t*>(BlitzenCore::BlitAllocLinear<char>(BlitzenCore::AllocationType::String, size));
            *byteCount = fread(*pBytesRead, 1, size, reinterpret_cast<FILE*>(handle.pHandle));
            if (*byteCount != size) 
            {
                return 0;
            }
            return 1;
        }
        return 0;
    }

    uint8_t FilesystemReadAllBytes(FileHandle& handle, BlitCL::StoragePointer<uint8_t, BlitzenCore::AllocationType::String>& bytes, 
    size_t* byteCount)
    {
        if(handle.pHandle)
        {
            // File size
            fseek(reinterpret_cast<FILE*>(handle.pHandle), 0, SEEK_END);
            uint64_t size = ftell(reinterpret_cast<FILE*>(handle.pHandle));

            rewind(reinterpret_cast<FILE*>(handle.pHandle));
            //BlitCL::String str{size};
            bytes.AllocateStorage(size);
            *byteCount = fread(bytes.Data(), 1, size, reinterpret_cast<FILE*>(handle.pHandle));
            if(*byteCount != size)
            {
                return 0;
            }
            return 1;
        }
        return 0;
    }
}