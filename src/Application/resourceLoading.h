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

        // This tag is for the shaders to know which image memory to access, when a render object uses this texture
        uint32_t textureTag;
    };

    struct MaterialStats
    {
        // Specify the diffuse color used on the shaders
        BlitML::vec4 diffuseColor;
        // Access the texture this material uses
        const char* diffuseMapName;
        // Allows shaders to access the saved data of the texture used by the diffuse map
        uint32_t diffuseMapTag;

        // This tag is for the shaders to know where in the material buffer this material can be accessed
        uint32_t materialTag;
    };

    uint8_t LoadResourceSystem(TextureStats* pTextures, MaterialStats* s_pMaterials);



    uint8_t LoadTextureFromFile(const char* filename);
    size_t GetTotalLoadedTexturesCount();


    size_t GetTotalLoadedMaterialCount();
}