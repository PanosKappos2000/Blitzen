#pragma once
#include "Platform/blitPlatformContext.h"
#include "Platform/Filesystem/blitCFILE.h"
#if defined(linux)

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#endif

constexpr size_t GCBlitStartOfFileOffset = 0;

namespace BlitzenPlatform
{
    enum class BLIT_MMF_RES : uint8_t
    {
        SUCCESS = 0,
        SUCCESS_FALLBACK = 1,

        FILE_CREATION_FAILED = 2,
        FILE_SIZE_INVALID = 3,
        FILE_SIZE_ZERO = 4,
        WRITE_SIZE_ZERO = 5,
        FILE_MAPPING_NULL = 6,
        FILE_MAPPING_VIEW_NULL = 7,
        MEMORY_ALLOCATION_FAILED = 8,

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
        case BLIT_MMF_RES::MEMORY_ALLOCATION_FAILED: return "{BLIT_MMF_RES_ERROR}: Manual Memory allocation failed\n";
		default: return "{BLIT_MMF_RES_ERROR}: Unknown error\n";
		}
	}

    // Returns true if the value of mmfResult means error
    inline bool CHECK_BLIT_MMF_RES_FOR_ERROR(BLIT_MMF_RES mmfResult)
    {
        return mmfResult != BLIT_MMF_RES::SUCCESS && mmfResult != BLIT_MMF_RES::SUCCESS_FALLBACK;
    }

    using FILE_MODE_FLAGS = uint32_t;
    using FILE_MODE_FLAG_BITS = uint8_t;

#if defined(_WIN32)

    class MEMORY_MAPPED_FILE_SCOPE
    {
    public:
        // Opens file for reading. Allocates the mapped pointer based on file size
        BLIT_MMF_RES OpenRead(const char* path);

        // Opens file for writing. Alloctes the mapped pointer based on write size passed
        BLIT_MMF_RES OpenWrite(const char* path, DWORD writeSize);

        // Opens file for both writing and reading. Allocates the mapped pointer based on write size passed. If it's zero, it allocates with file size
        BLIT_MMF_RES OpenGeneral(const char* path, DWORD writeSize);

        void Close();

        ~MEMORY_MAPPED_FILE_SCOPE();

        HANDLE m_hFile{ nullptr };                
        HANDLE m_pMapping{ nullptr };             
        LPVOID m_pFileView{ INVALID_HANDLE_VALUE };            
        DWORD m_fileSize{ 0 };
        DWORD m_endOffset{ 0 };

        inline bool IsRead() const { return m_mode & (FILE_MODE_FLAG_BITS)FileModes::Read; }

        inline bool IsWrite() const { return m_mode & (FILE_MODE_FLAG_BITS)FileModes::Write; }

        inline bool Failed() const { return m_pFileView == INVALID_HANDLE_VALUE; }

    private:
        FILE_MODE_FLAGS m_mode;
    };

#elif defined(linux)

    class MEMORY_MAPPED_FILE_SCOPE
    {
    public:
        BLIT_MMF_RES OpenRead(const char* path);

        BLIT_MMF_RES OpenWrite(const char* path, size_t writeSize);

        BLIT_MMF_RES OpenGeneral(const char* path, size_t writeSize);

        void Close();

        ~MEMORY_MAPPED_FILE_SCOPE();

        int32_t m_hFile{ -1 };            
        void* m_pFileView{ nullptr }; 
        size_t m_fileSize{ 0 };
        size_t m_endOffset{ 0 };

        inline bool IsRead() const { return m_mode & (FILE_MODE_FLAG_BITS)FileModes::Read; }

        inline bool IsWrite() const { return m_mode & (FILE_MODE_FLAG_BITS)FileModes::Write; }

        inline bool Failed() const { return m_pFileView == nullptr; }

    private:

        FILE_MODE_FLAGS m_mode{(FILE_MODE_FLAG_BITS)FileModes::NONE}; 
    };

#endif

    // Helper to read from the mapped memory
    bool ReadMemoryMappedFile(MEMORY_MAPPED_FILE_SCOPE& platformFile, size_t offset, size_t size, void* pDataRead);

    // Helper to write to the mapped memory
    bool WriteMemoryMappedFile(MEMORY_MAPPED_FILE_SCOPE& platformFile, size_t offset, size_t size, void* pData);
}