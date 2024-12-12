#include "resourceLoading.h"
#define STB_IMAGE_IMPLEMENTATION
#include "VendorCode/stb_image.h"

namespace BlitzenEngine
{
    uint8_t LoadResourceSystem(EngineResources& resources)
    {
        resources.textureTable.SetCapacity(BLIT_MAX_TEXTURE_COUNT);
        resources.materialTable.SetCapacity(BLIT_MAX_MATERIAL_COUNT);

        return 1;
    }



    /*---------------------------------
        Texture specific functions
    ----------------------------------*/

    uint8_t LoadTextureFromFile(EngineResources& resources, const char* filename, const char* texName)
    {
        TextureStats& current = resources.textures[resources.currentTextureIndex];

        current.pTextureData = stbi_load(filename, &(current.textureWidth), &(current.textureHeight), &(current.textureChannels), 4);

        uint8_t load = current.pTextureData != nullptr;

        // Go to the next element in the container, only if the texture was loaded successfully
        if(load)
        {
            current.textureTag = static_cast<uint32_t>(resources.currentTextureIndex);
            resources.textureTable.Set(texName, &current);
            resources.currentTextureIndex++;
        }
        // If the load failed give the default texture
        else
        {
            // TODO: Load the default texture so that the application does not fail
        }

        return load;
    }



    /*----------------------------------
        Material specific functions
    -----------------------------------*/

    void DefineMaterial(EngineResources& resources, BlitML::vec4& diffuseColor, float shininess, const char* diffuseMapName, 
    const char* specularMapName, const char* materialName)
    {
        MaterialStats& current = resources.materials[resources.currentMaterialIndex];

        current.diffuseColor = diffuseColor;
        current.shininess = shininess;
        current.diffuseMapName = diffuseMapName;
        current.diffuseMapTag = resources.textureTable.Get(diffuseMapName, &resources.textures[0])->textureTag;
        current.specularMapName = specularMapName;
        current.specularMapTag = resources.textureTable.Get(specularMapName, &resources.textures[0])->textureTag;
        current.materialTag = static_cast<uint32_t>(resources.currentMaterialIndex);
        resources.materialTable.Set(materialName, &current);
        resources.currentMaterialIndex++;
    }



    void LoadDefaultData(EngineResources& resources)
    {
        // Temporary shader data for tests on vulkan rendering
            resources.vertices.Resize(8);
            resources.indices.Resize(32);

            resources.vertices[0].position = BlitML::vec3(-0.5f, 0.5f, 0.f);
            resources.vertices[0].uvX = 0.f; 
            resources.vertices[0].uvY = 0.f;
            resources.vertices[0].normal = BlitML::vec3(0.f, 0.f, -1.f);

            resources.vertices[1].position = BlitML::vec3(-0.5f, -0.5f, 0.f);
            resources.vertices[1].uvX = 0.f; 
            resources.vertices[1].uvY = 1.f;
            resources.vertices[1].normal = BlitML::vec3(0.f, 0.f, -1.f);

            resources.vertices[2].position = BlitML::vec3(0.5f, -0.5f, 0.f);
            resources.vertices[2].uvX = 1.f;
            resources.vertices[2].uvY = 1.f;
            resources.vertices[2].normal = BlitML::vec3(0.f, 0.f, -1.f);

            resources.vertices[3].position = BlitML::vec3(0.5f, 0.5f, 0.f);
            resources.vertices[3].uvX = 1.f;
            resources.vertices[3].uvY = 0.f;
            resources.vertices[3].normal = BlitML::vec3(0.f, 0.f, -1.f);

            resources.vertices[4].position = BlitML::vec3(-0.5f, 0.5f, -0.5f);
            resources.vertices[4].uvX = 1.f; 
            resources.vertices[4].uvY = 0.f;
            resources.vertices[4].normal = BlitML::vec3(0.f, 0.f, -1.f);

            resources.vertices[5].position = BlitML::vec3(-0.5f, -0.5f, -0.5f);
            resources.vertices[5].uvX = 1.f; 
            resources.vertices[5].uvY = 1.f;
            resources.vertices[5].normal = BlitML::vec3(0.f, 0.f, -1.f);

            resources.vertices[6].position = BlitML::vec3(0.5f, -0.5f, -0.5f);
            resources.vertices[6].uvX = 0.f; 
            resources.vertices[6].uvY = 1.f;
            resources.vertices[6].normal = BlitML::vec3(0.f, 0.f, -1.f);

            resources.vertices[7].position = BlitML::vec3(0.5f, 0.5f, -0.5f);
            resources.vertices[7].uvX = 0.f; 
            resources.vertices[7].uvY = 0.f;
            resources.vertices[7].normal = BlitML::vec3(0.f, 0.f, -1.f);

            resources.indices[0] = 0;
            resources.indices[1] = 1;
            resources.indices[2] = 2;
            resources.indices[3] = 2;
            resources.indices[4] = 3;
            resources.indices[5] = 0;
            resources.indices[6] = 4;
            resources.indices[7] = 5;
            resources.indices[8] = 6;
            resources.indices[9] = 6;
            resources.indices[10] = 7;
            resources.indices[11] = 4;
            resources.indices[12] = 4;
            resources.indices[13] = 5;
            resources.indices[14] = 1;
            resources.indices[15] = 1;
            resources.indices[16] = 0;
            resources.indices[17] = 4;
            resources.indices[18] = 7;
            resources.indices[19] = 6;
            resources.indices[20] = 2;
            resources.indices[21] = 2;
            resources.indices[22] = 3;
            resources.indices[23] = 7;

            resources.meshes.Resize(1);
            resources.meshes[0].surfaces.Resize(4);

            resources.meshes[0].surfaces[0].firstIndex = 0;
            resources.meshes[0].surfaces[0].indexCount = 6;
            resources.meshes[0].surfaces[0].pMaterial = resources.materialTable.Get("loaded_material", &resources.materials[0]);

            resources.meshes[0].surfaces[1].firstIndex = 6;
            resources.meshes[0].surfaces[1].indexCount = 6;
            resources.meshes[0].surfaces[1].pMaterial = resources.materialTable.Get("loaded_material", &resources.materials[0]);

            resources.meshes[0].surfaces[2].firstIndex = 12;
            resources.meshes[0].surfaces[2].indexCount = 6;
            resources.meshes[0].surfaces[2].pMaterial = resources.materialTable.Get("loaded_material2", &resources.materials[0]);

            resources.meshes[0].surfaces[3].firstIndex = 18;
            resources.meshes[0].surfaces[3].indexCount = 6;
            resources.meshes[0].surfaces[3].pMaterial = resources.materialTable.Get("loaded_material", &resources.materials[0]);
    }
}