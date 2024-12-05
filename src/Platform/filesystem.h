#pragma once

#include "Core/blitLogger.h"

namespace BlitzenPlatform
{
    struct FileHandle
    {
        void* pHandle;
        uint8_t isValid;
    };

    enum class FileModes : uint8_t
    {
        Read = 0x1,
        Write = 0x2
    };

    // Determines if filepath exists
    uint8_t FilepathExists(const char* path);

    // Opens a file, returns 1 / true if successful
    uint8_t OpenFile(const char* path, FileModes mode, uint8_t binary, FileHandle& outHandle);
    void CloseFile(FileHandle& handle);

    // Read a single line from a file and saves it into a line buffer, return 1/true if successful
    uint8_t FilesystemReadLine(FileHandle& handle, size_t maxLength, char** lineBuffer, size_t* pLength);
    uint8_t FilesystemWriteLine(FileHandle& handle, const char* text);

    // Takes a file handle and the size of the data to read and saves the data read to a buffer and the size to an integer pointer
    uint8_t FilesystemRead(FileHandle& handle, size_t size, void* pDataRead, size_t* bytesRead);
    uint8_t FilesystemWrite(FileHandle& handle, size_t size, const void* pData, size_t* bitesWritten);

    // Takes a file handle and reads its content in byte form into a uint8_t buffer and its size into a byte count buffer
    uint8_t FilesystemReadAllBytes(FileHandle& handle, uint8_t** pBytesRead, size_t* byteCount);
}