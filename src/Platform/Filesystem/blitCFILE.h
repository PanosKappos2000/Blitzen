#pragma once
#include "BlitCL/blitString.h"

namespace BlitzenPlatform
{
    enum class FileModes : uint8_t
    {
        Read = 0x1,
        Write = 0x2
    };

    class C_FILE_SCOPE
    {
    public:
        bool Open(const char* path, FileModes mode, uint8_t binary);

        bool Open(const char* path, const char* mode);

        // Close the file manually
        void Close();

        ~C_FILE_SCOPE();
    public:
        FILE* m_pHandle{ nullptr };
    };

    bool FilepathExists(const char* path);

    bool FilesystemReadLine(C_FILE_SCOPE& handle, size_t maxLength, char** lineBuffer, size_t* pLength);

    bool FilesystemWriteLine(C_FILE_SCOPE& handle, const char* text);

    bool FilesystemRead(C_FILE_SCOPE& handle, size_t size, void* pDataRead, size_t* bytesRead);

    bool FilesystemWrite(C_FILE_SCOPE& handle, size_t size, const void* pData, size_t* bitesWritten);

    bool FilesystemReadAllBytes(C_FILE_SCOPE& handle, uint8_t** pBytesRead, size_t* byteCount);

    bool FilesystemReadAllBytes(C_FILE_SCOPE& handle, BlitCL::String& bytes, size_t* byteCount);
}