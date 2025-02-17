#pragma once
#include "Core/blitLogger.h"
#include "BlitzenMathLibrary/blitML.h"
#include "Core/blitzenContainerLibrary.h"
#include "Game/blitObject.h" // I probably do not want to include this here

namespace BlitzenEngine
{
    constexpr uint32_t ce_maxTextureCount = 5'000;

    constexpr uint32_t ce_maxMaterialCount = 10'000;

    constexpr uint8_t ce_primitiveSurfaceMaxLODCount = 8;

    constexpr uint32_t ce_maxMeshCount = 1'000'000; 

    constexpr uint32_t ce_maxRenderObjects = 5'000'000;

    struct TextureStats
    {
        // These things might not be necessary, since textures are immediately passed to the renderer that is requested
        int32_t textureWidth = 0;
        int32_t textureHeight = 0;
        int32_t textureChannels = 0;

        // TODO: this is no longer used, might want to remove later
        uint8_t* pTextureData;

        // This tag is for the shaders to know which image memory to access, when a material uses this texture
        uint32_t textureTag;
    };

    struct alignas(16) Vertex
    {
        // Vertex position (3 floats)
        BlitML::vec3 position;
        // Uv maps (2 halfs floats)
        uint16_t uvX, uvY;
        // Vertex normals (4 8-bit integers)
        uint8_t normalX, normalY, normalZ, normalW;
        // Tangents (4 8-bit integers)
        uint8_t tangentX, tangentY, tangentZ, tangentW;
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
        // Not using these 2 right now, might remove them when I'm not bored, but they (or something similar) will be used in the future
        BlitML::vec4 diffuseColor;
        float shininess;

        uint32_t albedoTag;// Index into the texture array, for the albedo map of the material

        uint32_t normalTag;// Index into the texture array, for the normal map of the material

        uint32_t specularTag;// Index into the texture array, for the specular map of the material

        uint32_t emissiveTag;// Index into the texture array for the emissive map of the material

        // TODO: I need to try removing this, it's a waster of space
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

        float error;
    };

    // Has information about a mesh surface that will be given to a GPU friendly struct, so that the GPU can draw each surface
    struct alignas(16) PrimitiveSurface
    {
        // Bounding sphere data, can be used for frustum culling and other operations
        BlitML::vec3 center;
        float radius;

        MeshLod meshLod[ce_primitiveSurfaceMaxLODCount];
        uint8_t lodCount = 0;

        // With the way obj files are loaded, this will be needed to index into the vertex buffer
        uint32_t vertexOffset;

        // Index into the material array to tell the fragment shader what material this primitive uses
        uint32_t materialId;

        uint8_t postPass = 0;
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

    // Accesses per draw data. A single draw has a unique transform and surface combination
    struct RenderObject
    {
        uint32_t transformId;
        uint32_t surfaceId;
    };

    // This struct holds every loaded resource that will be used for rendering all game objects
    struct RenderingResources
    {
        TextureStats textures[ce_maxTextureCount];
        BlitCL::HashMap<TextureStats> textureTable;
        size_t textureCount = 0;

        Material materials[ce_maxMaterialCount];
        BlitCL::HashMap<Material> materialTable;
        size_t materialCount = 0;

        // Arrays that hold all necessary geometry data
        BlitCL::DynamicArray<Vertex> vertices;
        BlitCL::DynamicArray<uint32_t> indices;
        BlitCL::DynamicArray<Meshlet> meshlets;
        BlitCL::DynamicArray<uint32_t> meshletData;

        // The data of every mesh allowed is in this fixed size array and the currentMeshIndex holds the current amount of loaded meshes
        Mesh meshes[ce_maxMeshCount];
        size_t meshCount = 0;

        // Each render object has a different transform held by this array 
        BlitCL::DynamicArray<MeshTransform> transforms;

        // Each mesh points to a continuous pack of elements of this surface array
        BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface> surfaces;

        // TODO: This is not a rendering resource, it should not be part of this struct
        GameObject objects[ce_maxRenderObjects];
        uint32_t objectCount;

        // All render objects are located here
        RenderObject renders[ce_maxRenderObjects];
        uint32_t renderObjectCount;        
    };

    uint8_t LoadRenderingResourceSystem(RenderingResources* pResources);


    // Takes a filename and loads at texture from it, and passes it to each renderer parameter that is not null
    uint8_t LoadTextureFromFile(RenderingResources* pResources, const char* filename, const char* texName, 
    void* pVulkan, void* pDirectx12);// The renderers are void* so that I do not expose their API in the .h file

    void LoadTestTextures(RenderingResources* pResources, uint8_t loadForVulkan, uint8_t loadForGL);


    void DefineMaterial(RenderingResources* pResources, BlitML::vec4& diffuseColor, float shininess, const char* diffuseMapName, 
    const char* specularMapName, const char* materialName);

    void LoadTestMaterials(RenderingResources* pResources, uint8_t loadForVulkan, uint8_t loadForGL);


    // Loads a mesh from an obj file
    uint8_t LoadMeshFromObj(RenderingResources* pResources, const char* filename);

    // Generates meshlet for a mesh or surface loaded using meshOptimizer library and converts it to the renderer's format
    size_t GenerateClusters(RenderingResources* pResources, 
    BlitCL::DynamicArray<Vertex>& vertices, 
    BlitCL::DynamicArray<uint32_t>& indices);

    // Takes the vertices and indices loaded for a mesh primitive from a file and converts the data to the renderer's format
    void LoadPrimitiveSurface(RenderingResources* pResources, 
    BlitCL::DynamicArray<Vertex>& vertices, 
    BlitCL::DynamicArray<uint32_t>& indices);


    // Placeholder to load some default resources while testing the systems
    void LoadTestGeometry(RenderingResources* pResources);


    // This function is used to load a default scene
    void CreateTestGameObjects(RenderingResources* pResources, uint32_t drawCount);


    // Calls some test functions to load a scene that tests the renderer's geometry rendering
    void LoadGeometryStressTest(RenderingResources* pResources, uint32_t drawCount, uint8_t loadForVulkan, uint8_t loadForGL);


    // Takes a path to a gltf file and loads the resources needed to render the scene
    // This function uses the cgltf library to load a .glb or .gltf scene
    // The repository can be found on https://github.com/jkuhlmann/cgltf
    uint8_t LoadGltfScene(RenderingResources* pResources, const char* path, uint8_t loadForVulkan, uint8_t loadForGL);
}