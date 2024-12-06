#include "resourceLoading.h"
#define STB_IMAGE_IMPLEMENTATION
#include "VendorCode/stb_image.h"

namespace BlitzenEngine
{
    static TextureStats* s_pTextures;
    static BlitCL::PointerTable<TextureStats>* s_pTextureTable;
    // Resource system expects the engine to create a fixed amount of default textures when it boots
    static size_t s_currentTextureIndex = BLIT_DEFAULT_TEXTURE_COUNT;

    static MaterialStats* s_pMaterials;
    static BlitCL::PointerTable<MaterialStats>* s_pMaterialTable;
    static size_t s_currentMaterialIndex = BLIT_DEFAULT_MATERIAL_COUNT;

    uint8_t LoadResourceSystem(TextureStats* pTextures, static BlitCL::PointerTable<TextureStats>* pTextureTable,
    MaterialStats* pMaterials, static BlitCL::PointerTable<MaterialStats>* pMaterialTable)
    {
        s_pTextures = pTextures;
        s_pTextureTable = pTextureTable;

        s_pMaterials = pMaterials;
        s_pMaterialTable = pMaterialTable;

        return s_pTextures != nullptr && s_pMaterials != nullptr;
    }



    /*---------------------------------
        Texture specific functions
    ----------------------------------*/

    uint8_t LoadTextureFromFile(const char* filename, const char* texName)
    {
        stbi_set_flip_vertically_on_load(1);
        s_pTextures[s_currentTextureIndex].pTextureData = stbi_load(filename, &(s_pTextures[s_currentTextureIndex].textureWidth), 
        &(s_pTextures[s_currentTextureIndex].textureHeight), 
        &(s_pTextures[s_currentTextureIndex].textureChannels), 4);

        uint8_t load = s_pTextures[s_currentTextureIndex].pTextureData != nullptr;

        // Go to the next element in the container, only if the texture was loaded successfully
        if(load)
        {
            s_pTextures[s_currentTextureIndex].textureTag = s_currentTextureIndex;
            s_pTextureTable->Set(texName, &(s_pTextures[s_currentTextureIndex]));
            s_currentTextureIndex++;
        }
        // If the load failed give the default texture
        else
        {
            // TODO: Load the default texture so that the application does not fail
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

    void DefineMaterial(BlitML::vec4& diffuseColor, const char* diffuseMapName)
    {
        MaterialStats& current = s_pMaterials[s_currentMaterialIndex];
        current.diffuseColor = diffuseColor;
        current.diffuseMapName = diffuseMapName;
        current.diffuseMapTag = s_pTextureTable->Get(diffuseMapName, &s_pTextures[0])->textureTag;
        current.materialTag = s_currentMaterialIndex;
        s_currentMaterialIndex++;
    }

    size_t GetTotalLoadedMaterialCount()
    {
        return s_currentMaterialIndex;
    }


}