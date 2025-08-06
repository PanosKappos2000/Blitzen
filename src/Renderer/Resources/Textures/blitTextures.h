#pragma once
#include <stdio.h>
#include "blitDDS.h"
#include "Platform/Filesystem/blitCFILE.h"
#include "BlitCL/blitHashMap.h"
#include "Renderer/Resources/blitShaderResources.h"
#include "Renderer/Resources/blitShaderShared.h"

namespace BlitzenEngine
{
    constexpr uint32_t GCMaxLoadedTextureCount = BLIT_MAX_WORLD_TEXTURE_RESOURCES;
    constexpr const char* GCRpfTextureSubfolder = "DDSTextures";
    constexpr size_t GCTextureHandleDataSize = 128 * 1024 * 1024;
    constexpr uint32_t GCWorldMapTextureNameMaxSize = 100;

    class TextureManager
    {
    public:
        
        uint32_t mTextureCount = 0;
        uint32_t mAllocatedCount;
        uint32_t mNotLoadedCount;
        size_t mTextureNamesBufferSize = 0;

        BLIT_OFFLINE_FUNC bool AddTexture(const char* textureName, const char* originalPath);
        BLIT_OFFLINE_FUNC bool AddTextureResourceFromScene(const char* sceneName, const char* originalPath, uint32_t textureID);
    };

    inline uint32_t FourCC(const char (&str)[5])
    {
	      return (unsigned(str[0]) << 0) | (unsigned(str[1]) << 8) | (unsigned(str[2]) << 16) | (unsigned(str[3]) << 24);
    }

    uint8_t OpenDDSImageFile(const char* filepath, DDS_HEADER& header, DDS_HEADER_DXT10& header10, BlitzenPlatform::C_FILE_SCOPE& handle);

    // Returns the amount of data needed to be allocated for the image data
    size_t GetDDSImageSizeBC(uint32_t width, uint32_t height, uint32_t levels, uint32_t blockSize);

    uint32_t GetDDSBlockSize(DDS_HEADER& header, DDS_HEADER_DXT10& header10);

    constexpr size_t CE_LOAD_DDS_IMAGE_DATA_ERROR_CODE = GCTextureHandleDataSize;
    size_t LoadDDSImageData(DDSFileContext& context, BlitzenPlatform::C_FILE_SCOPE& scopedFILE, void* pData, const char* filepath);
}