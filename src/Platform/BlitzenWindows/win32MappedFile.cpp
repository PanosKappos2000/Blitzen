#if defined(_WIN32)

#include "Platform/Common/blitMappedFile.h"


namespace BlitzenPlatform
{
    BLIT_MMF_RES MEMORY_MAPPED_FILE_SCOPE::OpenRead(const char* path)
    {
		m_mode = (FILE_MODE_FLAG_BITS)FileModes::Read;

        // CREATE
        m_hFile = CreateFileA(path, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (m_hFile == INVALID_HANDLE_VALUE)
        {
            return BLIT_MMF_RES::FILE_CREATION_FAILED;
        }

        // SIZE
        m_fileSize = GetFileSize(m_hFile, nullptr);
        if (m_fileSize == INVALID_FILE_SIZE)
        {
            return BLIT_MMF_RES::FILE_SIZE_ZERO;
        }
        if (m_fileSize == 0)
        {
            return BLIT_MMF_RES::FILE_SIZE_ZERO;
        }

        // MAPPING
        m_pMapping = CreateFileMappingA(m_hFile, nullptr, PAGE_READONLY, 0, m_fileSize, nullptr);
        if (!m_pMapping)
        {
            // TODO: Create helper to log these errors
            DWORD error = GetLastError();
            return BLIT_MMF_RES::FILE_MAPPING_NULL;
        }

        // MAPPING VIEW
        m_pFileView = MapViewOfFile(m_pMapping, FILE_MAP_READ, 0, 0, m_fileSize);
        if (!m_pFileView)
        {
            DWORD error = GetLastError();
            return BLIT_MMF_RES::FILE_MAPPING_VIEW_NULL;
        }

        return BLIT_MMF_RES::SUCCESS;
    }

    BLIT_MMF_RES MEMORY_MAPPED_FILE_SCOPE::OpenWrite(const char* path, DWORD writeSize)
    {
        m_mode = (FILE_MODE_FLAG_BITS)FileModes::Write;

        // CREATE
        m_hFile = CreateFileA(path, GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (m_hFile == INVALID_HANDLE_VALUE)
        {
            return BLIT_MMF_RES::FILE_CREATION_FAILED;
        }

        // SIZE
        if (writeSize == 0)
        {
            return BLIT_MMF_RES::WRITE_SIZE_ZERO;
        }
        m_fileSize = writeSize;

        // MAPPING
        m_pMapping = CreateFileMappingA(m_hFile, nullptr, PAGE_WRITECOPY, 0, m_fileSize, nullptr);
        if (!m_pMapping)
        {
           // RESET
           CloseHandle(m_hFile);
           m_hFile = nullptr;

            // TRY GENERAL
			if (this->OpenGeneral(path, writeSize) == BLIT_MMF_RES::SUCCESS)
			{
				return BLIT_MMF_RES::SUCCESS_FALLBACK;
			}
			else
			{
                DWORD error = GetLastError();
				return BLIT_MMF_RES::FILE_MAPPING_NULL;
			}
        }

        // Map the file to memory
        m_pFileView = MapViewOfFile(m_pMapping, FILE_MAP_WRITE, 0, 0, m_fileSize);
        if (!m_pFileView)
        {
            return BLIT_MMF_RES::FILE_MAPPING_VIEW_NULL;
        }

        return BLIT_MMF_RES::SUCCESS;
    }

    BLIT_MMF_RES MEMORY_MAPPED_FILE_SCOPE::OpenGeneral(const char* path, DWORD writeSize)
    {
        m_mode = (FILE_MODE_FLAG_BITS)FileModes::Read | (FILE_MODE_FLAG_BITS)FileModes::Write;

        // CREATE
        m_hFile = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (m_hFile == INVALID_HANDLE_VALUE)
        {
            return BLIT_MMF_RES::FILE_CREATION_FAILED;
        }

        if (writeSize == 0)
        {
            m_fileSize = GetFileSize(m_hFile, nullptr);
            if (m_fileSize == INVALID_FILE_SIZE)
            {
                return BLIT_MMF_RES::FILE_SIZE_INVALID;
            }
            if (m_fileSize == 0)
            {
                return BLIT_MMF_RES::FILE_SIZE_ZERO;
            }
        }
        else 
        {
            m_fileSize = writeSize;
        }

        // MAPPING
        m_pMapping = CreateFileMappingA(m_hFile, nullptr, PAGE_READWRITE, 0, m_fileSize, nullptr);
        if (!m_pMapping)
        {
            DWORD error = GetLastError();
            return BLIT_MMF_RES::FILE_MAPPING_NULL;
        }

        // MAPPING VIEW
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
        if (!platformFile.IsRead())
        {
            return false;
        }

        if (offset + size > platformFile.m_fileSize)
        {
            return false; 
        }

        // Copy the data from memory-mapped view
        BlitzenPlatform::PlatformMemCopy(pDataRead, reinterpret_cast<uint8_t*>(platformFile.m_pFileView) + offset, size);

        return true;
    }

    bool WriteMemoryMappedFile(MEMORY_MAPPED_FILE_SCOPE& platformFile, size_t offset, size_t size, void* pData)
    {
		if (!platformFile.IsWrite())
		{
			return false;
		}

        if (offset + size > platformFile.m_fileSize)
        {
            return false; 
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