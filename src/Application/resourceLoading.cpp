#include "textures.h"
#define STB_IMAGE_IMPLEMENTATION
#include "VendorCode/stb_image.h"

namespace BlitzenEngine
{
    static TextureStats* s_pTextures;
    // Resource system expects the engine to create a fixed amount of default textures when it boots
    static size_t s_currentTextureIndex = BLIT_DEFAULT_TEXTURE_COUNT;

    static MaterialStats* s_pMaterials;
    static size_t s_currentMaterialIndex = BLIT_DEFAULT_TEXTURE_COUNT;

    uint8_t LoadResourceSystem(TextureStats* pTextures, MaterialStats* pMaterials)
    {
        s_pTextures = pTextures;
        s_pMaterials = pMaterials;
        return s_pTextures != nullptr && s_pMaterials != nullptr;
    }



    /*---------------------------------
        Texture specific functions
    ----------------------------------*/

    uint8_t LoadTextureFromFile(const char* filename)
    {
        stbi_set_flip_vertically_on_load(1);
        s_pTextures[s_currentTextureIndex].pTextureData = stbi_load(filename, &(s_pTextures[s_currentTextureIndex].textureWidth), 
        &(s_pTextures[s_currentTextureIndex].textureHeight), 
        &(s_pTextures[s_currentTextureIndex].textureChannels), 4);

        uint8_t load = s_pTextures[s_currentTextureIndex].pTextureData != nullptr;

        // Go to the next element in the container, only if the texture was loaded successfully
        if(load)
        {
            s_currentTextureIndex++;
        }

        return load;
    }

    size_t GetTotalLoadedTexturesCount()
    {
        return s_currentTextureIndex;
    }



    /*----------------------------------
        Material specific functions
    -----------------------------------*/

    size_t GetTotalLoadedMaterialCount()
    {
        return s_currentMaterialIndex;
    }
}