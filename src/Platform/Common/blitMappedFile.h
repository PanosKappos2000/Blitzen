#pragma once
#include "Platform/blitPlatformContext.h"
#include "Platform/Filesystem/blitCFILE.h"

namespace BlitzenPlatform
{
    enum class BLIT_MMF_RES : uint8_t
    {
        SUCCESS = 0,

        FILE_CREATION_FAILED = 1,
        FILE_SIZE_INVALID = 2,
        FILE_SIZE_ZERO = 3,
        WRITE_SIZE_ZERO = 4,
        FILE_MAPPING_NULL = 5,
        FILE_MAPPING_VIEW_NULL = 6,

        BLIT_MMF_RES_MAX
    };

	inline const char* GET_BLIT_MMF_RES_ERROR_STR(BLIT_MMF_RES mmfResult)
	{
		switch (mmfResult)
		{
		case BLIT_MMF_RES::SUCCESS: return "Success";
		case BLIT_MMF_RES::FILE_CREATION_FAILED: return "{BLIT_MMF_RES_ERROR}: File creation failed\n";
		case BLIT_MMF_RES::FILE_SIZE_INVALID: return "{BLIT_MMF_RES_ERROR}: File size is invalid\n";
		case BLIT_MMF_RES::FILE_SIZE_ZERO: return "{BLIT_MMF_RES_ERROR}: File size is zero\n";
		case BLIT_MMF_RES::WRITE_SIZE_ZERO: return "{BLIT_MMF_RES_ERROR}: Write size is zero\n";
		case BLIT_MMF_RES::FILE_MAPPING_NULL: return "{BLIT_MMF_RES_ERROR}: File mapping handle is null\n";
		case BLIT_MMF_RES::FILE_MAPPING_VIEW_NULL: return "{BLIT_MMF_RES_ERROR}: File mapping view is null\n";
		default: return "{BLIT_MMF_RES_ERROR}: Unknown error\n";
		}
	}

    #if defined(_WIN32)
    struct MEMORY_MAPPED_FILE_SCOPE
    {
        BLIT_MMF_RES Open(const char* path, FileModes mode, DWORD writeSize);

        void Close();

        ~MEMORY_MAPPED_FILE_SCOPE();

        HANDLE m_hFile{ nullptr };                
        HANDLE m_pMapping{ nullptr };             
        LPVOID m_pFileView{ INVALID_HANDLE_VALUE };            
        DWORD m_fileSize{ 0 };             
    };

    #elif defined(linux)

    using MEMORY_MAPPED_FILE_SCOPE = C_FILE_SCOPE;// TEMPORARY

    #endif

    // Helper to read from the mapped memory
    bool ReadMemoryMappedFile(MEMORY_MAPPED_FILE_SCOPE& platformFile, size_t offset, size_t size, void* pDataRead);

    // Helper to write to the mapped memory
    bool WriteMemoryMappedFile(MEMORY_MAPPED_FILE_SCOPE& platformFile, size_t offset, size_t size, void* pData);
}