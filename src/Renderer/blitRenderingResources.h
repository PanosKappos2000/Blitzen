#pragma once
#include "Core/blitLogger.h"
#include "BlitzenMathLibrary/blitML.h"
#include "Core/blitzenContainerLibrary.h"
#include "Game/blitObject.h" // I probably do not want to include this here

#define BLIT_MAX_TEXTURE_COUNT      5000
#define BLIT_DEFAULT_TEXTURE_NAME   "blit_default_tex"
#define BLIT_TEXTURE_NAME_MAX_SIZE  512
#define BLIT_DEFAULT_TEXTURE_COUNT  1

#define BLIT_MAX_MATERIAL_COUNT     10000
#define BLIT_DEFAULT_MATERIAL_NAME  "blit_default_material"
#define BLIT_DEFAULT_MATERIAL_COUNT  1

#define BLIT_MAX_MESH_LOD           8
#define BLIT_MAX_MESH_COUNT         100'000

#define BLIT_MAX_OBJECTS            5'000'000

namespace BlitzenEngine
{
    struct TextureStats
    {
        int32_t textureWidth = 0;
        int32_t textureHeight = 0;
        int32_t textureChannels = 0;
        uint8_t* pTextureData;

        // This tag is for the shaders to know which image memory to access, when a material uses this texture
        uint32_t textureTag;
    };

    struct alignas(16) Vertex
    {
        BlitML::vec3 position;
        uint16_t uvX;
        BlitML::vec3 normal;
        uint16_t uvY;
    };

    struct alignas(16) Meshlet
    {
        // Bounding sphere for frustum culling
    	BlitML::vec3 center;
    	float radius;

        // This is for backface culling
    	int8_t cone_axis[3];
    	int8_t cone_cutoff;

    	uint32_t dataOffset; // Index into meshlet data
    	uint8_t vertexCount;
    	uint8_t triangleCount;
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
    struct alignas(16) MeshTransform
    {
        BlitML::vec3 pos;
        float scale;
        BlitML::quat orientation;
    };

    // This struct holds every loaded resource that will be used for rendering all game objects
    struct RenderingResources
    {
        TextureStats textures[BLIT_MAX_TEXTURE_COUNT];
        BlitCL::PointerTable<TextureStats> textureTable;
        size_t currentTextureIndex = BLIT_DEFAULT_TEXTURE_COUNT;

        Material materials[BLIT_MAX_MATERIAL_COUNT];
        BlitCL::PointerTable<Material> materialTable;
        size_t currentMaterialIndex = BLIT_DEFAULT_MATERIAL_COUNT;

        // Arrays that hold all necessary geometry data
        BlitCL::DynamicArray<Vertex> vertices;
        BlitCL::DynamicArray<uint32_t> indices;
        BlitCL::DynamicArray<Meshlet> meshlets;
        BlitCL::DynamicArray<uint32_t> meshletData;

        // The data of every mesh allowed is in this fixed size array and the currentMeshIndex holds the current amount of loaded meshes
        Mesh meshes[BLIT_MAX_MESH_COUNT];
        size_t currentMeshIndex = 0;

        // Each instance of a mesh has a different transform that is an element in this array
        BlitCL::DynamicArray<MeshTransform> transforms;

        // Each mesh points to a continuous pack of elements of this surface array
        BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface> surfaces;
        uint32_t currentSurfaceIndex = 0;// This might not be necessary since surfaces are placed in dynamic array

        BlitzenEngine::GameObject objects[BLIT_MAX_OBJECTS];
        uint32_t objectCount;
    };

    uint8_t LoadRenderingResourceSystem(RenderingResources* pResources);


    // Takes a filename and loads at texture from it, and passes it to each renderer parameter that is not null
    uint8_t LoadTextureFromFile(RenderingResources* pResources, const char* filename, const char* texName, 
    void* pVulkan, void* pDirectx12);// The renderers are void* so that I do not expose their API in the .h file

    void LoadTestTextures(RenderingResources* pResources, void* pVulkan, void* pDx12);


    void DefineMaterial(RenderingResources* pResources, BlitML::vec4& diffuseColor, float shininess, const char* diffuseMapName, 
    const char* specularMapName, const char* materialName);

    void LoadTestMaterials(RenderingResources* pResources, void* pVulkan, void* pDx12);


    uint8_t LoadMeshFromObj(RenderingResources* pResources, const char* filename, uint8_t buildMeshlets = 0);

    size_t LoadMeshlet(RenderingResources* pResources, BlitCL::DynamicArray<Vertex>& vertices, BlitCL::DynamicArray<uint32_t>& indices);


    // Placeholder to load some default resources while testing the systems
    void LoadDefaultData(RenderingResources* pResources);


    // This function is used to load a default scene
    void CreateTestGameObjects(RenderingResources* pResources, uint32_t drawCount);
}