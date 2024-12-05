#pragma once
#include "Core/blitLogger.h"
#include "BlitzenMathLibrary/blitMLTypes.h"

#define BLIT_MAX_TEXTURE_COUNT      5000
#define BLIT_DEFAULT_TEXTURE_NAME   "blit_default_tex"
#define BLIT_TEXTURE_NAME_MAX_SIZE  512
#define BLIT_DEFAULT_TEXTURE_COUNT  1

#define BLIT_MAX_MATERIAL_COUNT     10000
#define BLIT_DEFAULT_MATERIAL_NAME  "blit_default_material"
#define BLIT_DEFAULT_TEXTURE_COUNT  1

namespace BlitzenEngine
{
    struct TextureStats
    {
        int32_t textureWidth = 0;
        int32_t textureHeight = 0;
        int32_t textureChannels = 0;
        uint8_t* pTextureData;
    };

    struct MaterialStats
    {
        BlitML::vec4 diffuseColor;
        char diffuseMapName[BLIT_TEXTURE_NAME_MAX_SIZE];
    };

    uint8_t LoadResourceSystem(TextureStats* pTextures, MaterialStats* s_pMaterials);



    uint8_t LoadTextureFromFile(const char* filename);
    size_t GetTotalLoadedTexturesCount();


    size_t GetTotalLoadedMaterialCount();
}