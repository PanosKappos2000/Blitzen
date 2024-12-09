#pragma once
#include "Core/blitLogger.h"
#include "BlitzenMathLibrary/blitMLTypes.h"
#include "Core/blitzenContainerLibrary.h"

#define BLIT_MAX_TEXTURE_COUNT      5000
#define BLIT_DEFAULT_TEXTURE_NAME   "blit_default_tex"
#define BLIT_TEXTURE_NAME_MAX_SIZE  512
#define BLIT_DEFAULT_TEXTURE_COUNT  1

#define BLIT_MAX_MATERIAL_COUNT     10000
#define BLIT_DEFAULT_MATERIAL_NAME  "blit_default_material"
#define BLIT_DEFAULT_MATERIAL_COUNT  1

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

    // This is basically what a shader will draw, with each draw call
    struct PrimitiveSurface
    {
        uint32_t indexCount;
        uint32_t firstIndex;

        MaterialStats* pMaterial;
    };

    struct MeshAssets
    {
        BlitCL::DynamicArray<PrimitiveSurface> surfaces;
    };

    // This struct holds all loaded resources and is held by the engine and referenced by the loading system
    struct EngineResources
    {
        TextureStats textures[BLIT_MAX_TEXTURE_COUNT];
        BlitCL::PointerTable<TextureStats> textureTable;
        size_t currentTextureIndex = BLIT_DEFAULT_TEXTURE_COUNT;

        MaterialStats materials[BLIT_MAX_MATERIAL_COUNT];
        BlitCL::PointerTable<MaterialStats> materialTable;
        size_t currentMaterialIndex = BLIT_DEFAULT_MATERIAL_COUNT;

        BlitCL::DynamicArray<BlitML::Vertex> vertices;
        BlitCL::DynamicArray<uint32_t> indices;

        // Probably should have Mesh assets here instead of surfaces
        BlitCL::DynamicArray<MeshAssets> meshes;
    };

    uint8_t LoadResourceSystem(EngineResources& resources);



    uint8_t LoadTextureFromFile(EngineResources& resources, const char* filename, const char* texName);


    void DefineMaterial(EngineResources& resources, BlitML::vec4& diffuseColor, const char* diffuseMapName, const char* materialName);


    // Placeholder to load some default resources while testing the systems
    void LoadDefaultData(EngineResources& resources);
}