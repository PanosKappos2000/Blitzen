#if defined(_WIN32)
#include "Platform/Common/blitMappedFile.h"


namespace BlitzenPlatform
{
    BLIT_MMF_RES MEMORY_MAPPED_FILE_SCOPE::Open(const char* path, FileModes mode, DWORD writeSize)
    {
        DWORD accessFlags = 0;
        DWORD creationDisposition = OPEN_EXISTING;

        if (mode == FileModes::Read)
        {
            accessFlags = GENERIC_READ;
        }
        else if (mode == FileModes::Write)
        {
            accessFlags = GENERIC_WRITE;

            // Creates file if it does not exist
            // Might want to put a parameter for this
            creationDisposition = OPEN_ALWAYS;
        }

        m_hFile = CreateFileA(path, accessFlags, 0, nullptr, creationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (m_hFile == INVALID_HANDLE_VALUE)
        {
            return BLIT_MMF_RES::FILE_CREATION_FAILED;
        }

        // GET SIZE FOR READ
        if (mode == FileModes::Read)
        {
            m_fileSize = GetFileSize(m_hFile, nullptr);
            if (m_fileSize == INVALID_FILE_SIZE)
            {
                return BLIT_MMF_RES::FILE_SIZE_ZERO;
            }
            if (m_fileSize == 0)
            {
                return BLIT_MMF_RES::FILE_SIZE_ZERO;
            }
        }
        // SET SIZE FOR WRITE
        else if (mode == FileModes::Write)
        {
            if (writeSize == 0)
            {
                return BLIT_MMF_RES::WRITE_SIZE_ZERO;
            }

            m_fileSize = writeSize;
        }
        else
        {
            return BLIT_MMF_RES::BLIT_MMF_RES_MAX;
        }

        DWORD ffProtect{ mode == FileModes::Read ? (DWORD)PAGE_READONLY : (DWORD)PAGE_READWRITE };

        // MEMORY MAPPING
        m_pMapping = CreateFileMappingA(m_hFile, nullptr, ffProtect, 0, m_fileSize, nullptr);
        if (!m_pMapping)
        {
            if (mode == FileModes::Write)
            {
                // RESET
                CloseHandle(m_hFile);
                m_hFile = nullptr;

                // TRY GENERAL
                accessFlags = GENERIC_WRITE | GENERIC_READ;
                ffProtect = PAGE_READWRITE;

                m_hFile = CreateFileA(path, accessFlags, 0, nullptr, creationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (m_hFile == INVALID_HANDLE_VALUE)
                {
                    return BLIT_MMF_RES::FILE_CREATION_FAILED;
                }

                // MEMORY MAPPING
                m_pMapping = CreateFileMappingA(m_hFile, nullptr, ffProtect, 0, m_fileSize, nullptr);
                if (!m_pMapping)
                {
                    DWORD error = GetLastError();
                    return BLIT_MMF_RES::FILE_MAPPING_NULL;
                }
            }
            else
            {
                DWORD error = GetLastError();
                return BLIT_MMF_RES::FILE_MAPPING_NULL;
            }
        }

        // Map the file to memory
        m_pFileView = MapViewOfFile(m_pMapping, FILE_MAP_ALL_ACCESS, 0, 0, m_fileSize);
        if (!m_pFileView)
        {
            return BLIT_MMF_RES::FILE_MAPPING_VIEW_NULL;
        }

        return BLIT_MMF_RES::SUCCESS; // Successfully opened the file
    }

    void MEMORY_MAPPED_FILE_SCOPE::Close()
    {
        if (m_pFileView)
        {
            UnmapViewOfFile(m_pFileView);
            m_pFileView = nullptr;
        }

        if (m_pMapping)
        {
            CloseHandle(m_pMapping);
            m_pMapping = nullptr;
        }

        if (m_hFile != nullptr)
        {
            CloseHandle(m_hFile);
            m_hFile = nullptr;
        }
    }

    MEMORY_MAPPED_FILE_SCOPE::~MEMORY_MAPPED_FILE_SCOPE()
    {
        Close();
    }

    bool ReadMemoryMappedFile(MEMORY_MAPPED_FILE_SCOPE& platformFile, size_t offset, size_t size, void* pDataRead)
    {
        if (offset + size > platformFile.m_fileSize)
        {
            return false; // Read exceeds file size
        }

        // Copy the data from memory-mapped view
        BlitzenPlatform::PlatformMemCopy(pDataRead, reinterpret_cast<uint8_t*>(platformFile.m_pFileView) + offset, size);

        return true;
    }

    bool WriteMemoryMappedFile(MEMORY_MAPPED_FILE_SCOPE& platformFile, size_t offset, size_t size, void* pData)
    {
        if (offset + size > platformFile.m_fileSize)
        {
            return false; // Write exceeds file size
        }

        // Copy the data into the memory-mapped view
        BlitzenPlatform::PlatformMemCopy(reinterpret_cast<uint8_t*>(platformFile.m_pFileView) + offset, pData, size);

        if (platformFile.m_endOffset < offset + size)
        {
            platformFile.m_endOffset = (DWORD)(offset + size);
        }

        return true;
    }
}
#endif