#if defined(linux)

#include "Platform/Common/blitMappedFile.h"

namespace BlitzenPlatform
{
    BLIT_MMF_RES MEMORY_MAPPED_FILE_SCOPE::OpenRead(const char* path)
    {
        m_mode = (FILE_MODE_FLAG_BITS)FileModes::Read;

        // open
        m_hFile = open(path, O_RDONLY);
        if (m_hFile == -1) 
        {
            return BLIT_MMF_RES::FILE_CREATION_FAILED;
        }

        // size
        m_fileSize = lseek(m_hFile, 0, SEEK_END);
        if (m_fileSize == (off_t)-1)
        {
            return BLIT_MMF_RES::FILE_SIZE_INVALID;
        } 
        if(m_fileSize == 0) 
        {
            return BLIT_MMF_RES::FILE_SIZE_ZERO;
        }

        // map
        m_pFileView = mmap(nullptr, m_fileSize, PROT_READ, MAP_PRIVATE, m_hFile, 0);
        if (m_pFileView == MAP_FAILED) 
        {
            m_pFileView = nullptr;
            return BLIT_MMF_RES::FILE_MAPPING_NULL;
        }

        return BLIT_MMF_RES::SUCCESS;
    }

    BLIT_MMF_RES MEMORY_MAPPED_FILE_SCOPE::OpenWrite(const char* path, size_t writeSize)
    {
        m_mode = (FILE_MODE_FLAG_BITS)FileModes::Write;

        // open
        m_hFile = open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
        if (m_hFile == -1) 
        {
            return BLIT_MMF_RES::FILE_CREATION_FAILED;
        }

        if (writeSize == 0) 
        {
            return BLIT_MMF_RES::FILE_SIZE_ZERO;
        }
        m_fileSize = writeSize;

        // mapping
        m_pFileView = mmap(nullptr, m_fileSize, PROT_WRITE, MAP_SHARED, m_hFile, 0);
        if (m_pFileView == MAP_FAILED) 
        {
            m_pFileView = nullptr;
            return BLIT_MMF_RES::FILE_MAPPING_NULL;
        }

        return BLIT_MMF_RES::SUCCESS;
    }

    BLIT_MMF_RES MEMORY_MAPPED_FILE_SCOPE::OpenGeneral(const char* path, size_t writeSize)
    {
        m_mode = (FILE_MODE_FLAG_BITS)FileModes::Read | (FILE_MODE_FLAG_BITS)FileModes::Write;

        // open
        m_hFile = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if (m_hFile == -1) 
        {
            return BLIT_MMF_RES::FILE_CREATION_FAILED;
        }

        // size
        if(writeSize == 0)
        {
            m_fileSize = lseek(m_hFile, 0, SEEK_END);
            if (m_fileSize == (off_t)-1)
            {
                return BLIT_MMF_RES::FILE_SIZE_INVALID;
            } 
            if(m_fileSize == 0) 
            {
                return BLIT_MMF_RES::FILE_SIZE_ZERO;
            }
        }
        else
        {
            m_fileSize = writeSize;
        }

        // mapping
        m_pFileView = mmap(nullptr, m_fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, m_hFile, 0);
        if (m_pFileView == MAP_FAILED) 
        {
            m_pFileView = nullptr;
            return BLIT_MMF_RES::FILE_MAPPING_NULL;
        }

        return BLIT_MMF_RES::SUCCESS;
    }

    void MEMORY_MAPPED_FILE_SCOPE::Close()
    {
        if (m_pFileView) 
        {
            munmap(m_pFileView, m_fileSize);
            m_pFileView = nullptr; 
        }

        if (m_hFile != -1) 
        {
            close(m_hFile); 
            m_hFile = -1;
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
            platformFile.m_endOffset = (size_t)(offset + size);
        }

        return true;
    }
}

#endif