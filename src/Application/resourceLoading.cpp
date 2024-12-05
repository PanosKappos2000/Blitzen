#include "textures.h"
#define STB_IMAGE_IMPLEMENTATION
#include "VendorCode/stb_image.h"

namespace BlitzenEngine
{
    static TextureStats* s_pTextures;
    // Resource system expects the engine to create one default texture during loading
    static size_t s_currentIndex = 1;

    uint8_t LoadResourceSystem(TextureStats* pTextures)
    {
        s_pTextures = &(pTextures[0]);
        return s_pTextures != nullptr;
    }

    uint8_t LoadTextureFromFile(const char* filename)
    {
        stbi_set_flip_vertically_on_load(1);
        s_pTextures[s_currentIndex].pTextureData = stbi_load(filename, &(s_pTextures[s_currentIndex].textureWidth), 
        &(s_pTextures[s_currentIndex].textureHeight), 
        &(s_pTextures[s_currentIndex].textureChannels), 4);

        uint8_t load = s_pTextures[s_currentIndex].pTextureData != nullptr;

        // Go to the next element in the container, only if the texture was loaded successfully
        if(load)
        {
            s_currentIndex++;
        }

        return load;
    }

    size_t GetTotalLoadedTexturesCount()
    {
        return s_currentIndex;
    }
}