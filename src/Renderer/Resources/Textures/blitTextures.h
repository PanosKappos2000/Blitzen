#pragma once
#include <stdio.h>
#include "blitDDS.h"
#include "Platform/filesystem.h"
#include "BlitCL/blitHashMap.h"
#include "Renderer/Resources/renderingResourcesTypes.h"

namespace BlitzenEngine
{
    struct TextureManager
    {
        Material m_materials[BlitzenCore::Ce_MaxMaterialCount];
        BlitCL::HashMap<Material*> m_pMaterialTable;
        uint32_t m_materialCount;

        BlitCL::HashMap<uint32_t> m_textureIDMap;
        uint32_t m_textureCount{ 0 };

        bool AddTexture(const char* textureName);

        bool AddMaterial(uint32_t albedoId, uint32_t normalId, uint32_t specularId, uint32_t emissiveId, const char* name = "BLIT_DO_NOT_ADD_TO_MATERIAL_MAP");
    };

    inline unsigned int FourCC(const char (&str)[5])
    {
	      return (unsigned(str[0]) << 0) | (unsigned(str[1]) << 8) | (unsigned(str[2]) << 16) | (unsigned(str[3]) << 24);
    }

    uint8_t OpenDDSImageFile(const char* filepath, DDS_HEADER& header, DDS_HEADER_DXT10& header10, BlitzenPlatform::FileHandle& handle);

    // Returns the amount of data needed to be allocated for the image data
    size_t GetDDSImageSizeBC(unsigned int width, unsigned int height, unsigned int levels, unsigned int blockSize);

    uint32_t GetDDSBlockSize(DDS_HEADER& header, DDS_HEADER_DXT10& header10);
}