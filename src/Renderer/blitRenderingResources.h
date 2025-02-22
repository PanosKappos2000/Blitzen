#pragma once
#include "Core/blitLogger.h"
#include "BlitzenMathLibrary/blitML.h"
#include "Core/blitzenContainerLibrary.h"
#include "Game/blitObject.h" // I probably do not want to include this here
#include <string>

namespace BlitzenEngine
{
    constexpr uint32_t ce_maxTextureCount = 5'000;

    constexpr uint32_t ce_maxMaterialCount = 10'000;

    constexpr uint8_t ce_primitiveSurfaceMaxLODCount = 8;

    constexpr uint32_t ce_maxMeshCount = 1'000'000; 

    constexpr uint32_t ce_maxRenderObjects = 5'000'000;

    #ifdef BLITZEN_CLUSTER_CULLING
        constexpr uint8_t ce_buildClusters = 1;
    #else
        constexpr uint8_t ce_buildClusters = 0;
    #endif 

    struct TextureStats
    {
        std::string filepath;

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
        // Holds all textures. No dynamic allocation.
        TextureStats textures[ce_maxTextureCount];
        BlitCL::HashMap<TextureStats> textureTable;
        size_t textureCount = 0;

        // Holds all materials. No dynamic allocation. Includes hashmap for separate access
        Material materials[ce_maxMaterialCount];
        BlitCL::HashMap<Material> materialTable;
        size_t materialCount = 0;


        /*
            Per primitive data
        */
        // Holds all the primitives / surfaces
        BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface> surfaces;

        // Holds the vertex count of each primitive. This does not need to be passed to shader for now. But I do need it for ray tracing
        BlitCL::DynamicArray<uint32_t> primitiveVertexCounts;

        // Holds the vertices of all the primitives that were loaded
        BlitCL::DynamicArray<Vertex> vertices;

        // Holds the indices of all the primitives that were loaded
        BlitCL::DynamicArray<uint32_t> indices;

        // Holds all clusters for all the primitives that were loaded
        BlitCL::DynamicArray<Meshlet> meshlets;

        // Holds the meshlet indices to index into the clusters above
        BlitCL::DynamicArray<uint32_t> meshletData;


        /*
            Per instance data
        */
        // Holds the transforms of every render object / instance on the scene 
        BlitCL::DynamicArray<MeshTransform> transforms;

        // Holds all the render objects / primitives. They index into one primitive and one transform each
        RenderObject renders[ce_maxRenderObjects];
        uint32_t renderObjectCount; 
        
        // Holds the meshes that were loaded for the scene. Meshes are a collection of primitives. TODO: Put these on a separate struct (maybe)
        Mesh meshes[ce_maxMeshCount];
        size_t meshCount = 0;

        // TODO: This is not a rendering resource, it should not be part of this struct
        GameObject objects[ce_maxRenderObjects];
        uint32_t objectCount;
    };

    // Draw context needs to be given to draw frame function, so that it can update uniform values
    struct DrawContext
    {
        // The shaders have a struct that aligns with one of the structs of the camera class.
        // It holds crucial data for culling and rendering like view matrix, frustum planes, screen coordinate lodTarget and more
        void* pCamera;

        // Vulkan needs to know how many objects are being drawn
        uint32_t drawCount;

        // Debug values
        uint8_t bOcclusionCulling;
        uint8_t bLOD;

        inline DrawContext(void* pCam, uint32_t dc, uint8_t bOC = 1, uint8_t bLod = 1) 
        : pCamera(pCam), drawCount(dc), bOcclusionCulling{bOC}, bLOD{bLod} {}
    };

    uint8_t LoadRenderingResourceSystem(RenderingResources* pResources);

    void DefineMaterial(RenderingResources* pResources, BlitML::vec4& diffuseColor, float shininess, const char* diffuseMapName, 
    const char* specularMapName, const char* materialName);

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
    void LoadGeometryStressTest(RenderingResources* pResources, uint32_t drawCount);
    
    uint8_t LoadTextureFromFile(RenderingResources* pResources, const char* filename, const char* texName);

    // Takes a path to a gltf file and loads the resources needed to render the scene
    // This function uses the cgltf library to load a .glb or .gltf scene
    // The repository can be found on https://github.com/jkuhlmann/cgltf
    uint8_t LoadGltfScene(RenderingResources* pResources, const char* path);
}