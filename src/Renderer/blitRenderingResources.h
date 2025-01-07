#pragma once
#include "Core/blitLogger.h"
#include "BlitzenMathLibrary/blitML.h"
#include "Core/blitzenContainerLibrary.h"

#define BLIT_MAX_TEXTURE_COUNT      5000
#define BLIT_DEFAULT_TEXTURE_NAME   "blit_default_tex"
#define BLIT_TEXTURE_NAME_MAX_SIZE  512
#define BLIT_DEFAULT_TEXTURE_COUNT  1

#define BLIT_MAX_MATERIAL_COUNT     10000
#define BLIT_DEFAULT_MATERIAL_NAME  "blit_default_material"
#define BLIT_DEFAULT_MATERIAL_COUNT  1

#define BLIT_MAX_MESH_LOD           8
#define BLIT_MAX_MESH_COUNT         100'000

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

    // Passed to the GPU as a unified storage buffer. Part of Material stats
    struct alignas(16) Material
    {
        BlitML::vec4 diffuseColor;
        float shininess;

        uint32_t diffuseTextureTag;
        uint32_t specularTextureTag;

        uint32_t materialId;
    };

    struct MeshLod
    {
        // The amount of indices that have to be iterated over after the first index, in order to draw the surface with this lod
        uint32_t indexCount;
        // The index of the first element in the index buffer for this mesh lod
        uint32_t firstIndex;

        uint32_t meshletCount = 0;
        uint32_t firstMeshlet;
    };

    // Has information about a mesh surface that will be given to a GPU friendly struct, so that the GPU can draw each surface
    struct alignas(16) PrimitiveSurface
    {
        MeshLod meshLod[BLIT_MAX_MESH_LOD];
        uint32_t lodCount = 0;

        // With the way obj files are loaded, this will be needed to index into the vertex buffer
        uint32_t vertexOffset;

        // Data need by a mesh shader to draw a surface
        uint32_t meshletCount = 0;
        uint32_t firstMeshlet;

        // Bounding sphere data, can be used for frustum culling and other operations
        BlitML::vec3 center;
        float radius;

        uint32_t materialId;

        uint32_t surfaceId;
    };

    struct Mesh
    {
        uint32_t firstSurface;
        uint32_t surfaceCount = 0;
    };

    // Data that can vary for 2 different object with the same or different objects
    struct alignas(16) MeshInstance
    {
        BlitML::vec3 pos;
        float scale;
        BlitML::quat orientation;
    };

    // This struct holds all loaded resources and is held by the engine and referenced by the loading system
    struct EngineResources
    {
        TextureStats textures[BLIT_MAX_TEXTURE_COUNT];
        BlitCL::PointerTable<TextureStats> textureTable;
        size_t currentTextureIndex = BLIT_DEFAULT_TEXTURE_COUNT;

        Material materials[BLIT_MAX_MATERIAL_COUNT];
        BlitCL::PointerTable<Material> materialTable;
        size_t currentMaterialIndex = BLIT_DEFAULT_MATERIAL_COUNT;

        BlitCL::DynamicArray<BlitML::Vertex> vertices;
        BlitCL::DynamicArray<uint32_t> indices;
        BlitCL::DynamicArray<BlitML::Meshlet> meshlets;

        // Every mesh allowed is place here
        Mesh meshes[BLIT_MAX_MESH_COUNT];
        size_t currentMeshIndex = 0;
        uint32_t currentSurfaceIndex = 0;

        // Each mesh points to a continuous pack of elements of this surface array
        BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface> surfaces;
    };

    uint8_t LoadResourceSystem(EngineResources& resources);



    uint8_t LoadTextureFromFile(EngineResources& resources, const char* filename, const char* texName);


    void DefineMaterial(EngineResources& resources, BlitML::vec4& diffuseColor, float shininess, const char* diffuseMapName, 
    const char* specularMapName, const char* materialName);


    uint8_t LoadMeshFromObj(EngineResources& resources, const char* filename, uint8_t buildMeshlets = 0);

    size_t LoadMeshlet(EngineResources& resoureces, BlitCL::DynamicArray<BlitML::Vertex>& vertices, BlitCL::DynamicArray<uint32_t>& indices);


    // Placeholder to load some default resources while testing the systems
    void LoadDefaultData(EngineResources& resources);
}