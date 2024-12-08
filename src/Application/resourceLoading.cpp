#include "resourceLoading.h"
#define STB_IMAGE_IMPLEMENTATION
#include "VendorCode/stb_image.h"

namespace BlitzenEngine
{
    EngineResources* s_pResources;

    // Resource system expects the engine to create a fixed amount of default textures when it boots
    static size_t s_currentTextureIndex = BLIT_DEFAULT_TEXTURE_COUNT;

    static size_t s_currentMaterialIndex = BLIT_DEFAULT_MATERIAL_COUNT;

    uint8_t LoadResourceSystem(EngineResources* pResources)
    {
        s_pResources = pResources;

        s_pResources->textureTable.SetCapacity(BLIT_MAX_TEXTURE_COUNT);
        s_pResources->materialTable.SetCapacity(BLIT_MAX_MATERIAL_COUNT);

        return s_pResources != nullptr;
    }



    /*---------------------------------
        Texture specific functions
    ----------------------------------*/

    uint8_t LoadTextureFromFile(const char* filename, const char* texName)
    {
        TextureStats& current = s_pResources->textures[s_currentTextureIndex];

        stbi_set_flip_vertically_on_load(1);
        current.pTextureData = stbi_load(filename, 
        &(current.textureWidth), 
        &(current.textureHeight), 
        &(current.textureChannels), 4);

        uint8_t load = current.pTextureData != nullptr;

        // Go to the next element in the container, only if the texture was loaded successfully
        if(load)
        {
            current.textureTag = static_cast<uint32_t>(s_currentTextureIndex);
            s_pResources->textureTable.Set(texName, &current);
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

    void DefineMaterial(BlitML::vec4& diffuseColor, const char* diffuseMapName, const char* materialName)
    {
        MaterialStats& current = s_pResources->materials[s_currentMaterialIndex];

        current.diffuseColor = diffuseColor;
        current.diffuseMapName = diffuseMapName;
        current.diffuseMapTag = s_pResources->textureTable.Get(diffuseMapName, &s_pResources->textures[0])->textureTag;
        current.materialTag = static_cast<uint32_t>(s_currentMaterialIndex);
        s_pResources->materialTable.Set(materialName, &current);
        s_currentMaterialIndex++;
    }

    size_t GetTotalLoadedMaterialCount()
    {
        return s_currentMaterialIndex;
    }



    void LoadDefaultData()
    {
        // Temporary shader data for tests on vulkan rendering
            s_pResources->vertices.Resize(8);
            s_pResources->indices.Resize(32);

            s_pResources->vertices[0].position = BlitML::vec3(-0.5f, 0.5f, 0.f);
            s_pResources->vertices[0].uvX = 0.f; 
            s_pResources->vertices[0].uvY = 0.f;
            s_pResources->vertices[1].position = BlitML::vec3(-0.5f, -0.5f, 0.f);
            s_pResources->vertices[1].uvX = 0.f; 
            s_pResources->vertices[1].uvY = 1.f;
            s_pResources->vertices[2].position = BlitML::vec3(0.5f, -0.5f, 0.f);
            s_pResources->vertices[2].uvX = 1.f;
            s_pResources->vertices[2].uvY = 1.f;
            s_pResources->vertices[3].position = BlitML::vec3(0.5f, 0.5f, 0.f);
            s_pResources->vertices[3].uvX = 1.f;
            s_pResources->vertices[3].uvY = 0.f;
            s_pResources->vertices[4].position = BlitML::vec3(-0.5f, 0.5f, -0.5f);
            s_pResources->vertices[4].uvX = 1.f; 
            s_pResources->vertices[4].uvY = 0.f;
            s_pResources->vertices[5].position = BlitML::vec3(-0.5f, -0.5f, -0.5f);
            s_pResources->vertices[5].uvX = 1.f; 
            s_pResources->vertices[5].uvY = 1.f;
            s_pResources->vertices[6].position = BlitML::vec3(0.5f, -0.5f, -0.5f);
            s_pResources->vertices[6].uvX = 0.f; 
            s_pResources->vertices[6].uvY = 1.f;
            s_pResources->vertices[7].position = BlitML::vec3(0.5f, 0.5f, -0.5f);
            s_pResources->vertices[7].uvX = 0.f; 
            s_pResources->vertices[7].uvY = 0.f;

            s_pResources->indices[0] = 0;
            s_pResources->indices[1] = 1;
            s_pResources->indices[2] = 2;
            s_pResources->indices[3] = 2;
            s_pResources->indices[4] = 3;
            s_pResources->indices[5] = 0;
            s_pResources->indices[6] = 4;
            s_pResources->indices[7] = 5;
            s_pResources->indices[8] = 6;
            s_pResources->indices[9] = 6;
            s_pResources->indices[10] = 7;
            s_pResources->indices[11] = 4;
            s_pResources->indices[12] = 4;
            s_pResources->indices[13] = 5;
            s_pResources->indices[14] = 1;
            s_pResources->indices[15] = 1;
            s_pResources->indices[16] = 0;
            s_pResources->indices[17] = 4;
            s_pResources->indices[18] = 7;
            s_pResources->indices[19] = 6;
            s_pResources->indices[20] = 2;
            s_pResources->indices[21] = 2;
            s_pResources->indices[22] = 3;
            s_pResources->indices[23] = 7;

            s_pResources->meshes.Resize(1);
            s_pResources->meshes[0].surfaces.Resize(4);

            s_pResources->meshes[0].surfaces[0].firstIndex = 0;
            s_pResources->meshes[0].surfaces[0].indexCount = 6;
            s_pResources->meshes[0].surfaces[0].pMaterial = s_pResources->materialTable.Get("loaded_material", &s_pResources->materials[0]);

            s_pResources->meshes[0].surfaces[1].firstIndex = 6;
            s_pResources->meshes[0].surfaces[1].indexCount = 6;
            s_pResources->meshes[0].surfaces[1].pMaterial = s_pResources->materialTable.Get("loaded_material", &s_pResources->materials[0]);

            s_pResources->meshes[0].surfaces[2].firstIndex = 12;
            s_pResources->meshes[0].surfaces[2].indexCount = 6;
            s_pResources->meshes[0].surfaces[2].pMaterial = s_pResources->materialTable.Get("loaded_material2", &s_pResources->materials[0]);

            s_pResources->meshes[0].surfaces[3].firstIndex = 18;
            s_pResources->meshes[0].surfaces[3].indexCount = 6;
            s_pResources->meshes[0].surfaces[3].pMaterial = s_pResources->materialTable.Get("loaded_material", &s_pResources->materials[0]);
    }
}