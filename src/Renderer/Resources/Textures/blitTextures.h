#pragma once
#include <stdio.h>
#include "blitDDS.h"
#include "Platform/Filesystem/blitCFILE.h"
#include "BlitCL/blitHashMap.h"
#include "Renderer/Resources/blitShaderResources.h"

namespace BlitzenEngine
{
    class TextureManager
    {
    public:
        
        BlitCL::HashMap<uint32_t> m_textureIDMap;
        uint32_t m_textureCount{ 0 };

        BLIT_STRAIGHTHANDLE m_singleTextureHandle;

        void ALLOC();

        ~TextureManager();

        bool AddTexture(const char* textureName);
    };

    inline unsigned int FourCC(const char (&str)[5])
    {
	      return (unsigned(str[0]) << 0) | (unsigned(str[1]) << 8) | (unsigned(str[2]) << 16) | (unsigned(str[3]) << 24);
    }

    uint8_t OpenDDSImageFile(const char* filepath, DDS_HEADER& header, DDS_HEADER_DXT10& header10, BlitzenPlatform::C_FILE_SCOPE& handle);

    // Returns the amount of data needed to be allocated for the image data
    size_t GetDDSImageSizeBC(unsigned int width, unsigned int height, unsigned int levels, unsigned int blockSize);

    uint32_t GetDDSBlockSize(DDS_HEADER& header, DDS_HEADER_DXT10& header10);

    constexpr size_t CE_LOAD_DDS_IMAGE_DATA_ERROR_CODE = BlitzenCore::CE_TEXTURE_DATA_HANDLE_SIZE;
    size_t LoadDDSImageData(DDS_HEADER& header, DDS_HEADER_DXT10& header10, BlitzenPlatform::C_FILE_SCOPE& scopedFILE, BLIT_DXGI_FORMAT_COPY& format, void* pData, uint32_t& blockSize, const char* filepath);
}