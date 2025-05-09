#pragma once
#include "Engine/blitzenEngine.h"
#include "BlitzenMathLibrary/blitML.h"
#include <string>

namespace BlitzenEngine
{
    struct TextureStats
    {
		std::string filepath;// Probably useless, but whatever
        uint8_t* pTextureData;// Probably useless, but whatever
        uint32_t textureTag;// Do I even use this piece of crap?
    };

    struct alignas(16) Vertex
    {
        BlitML::vec3 position;
        float uvX, uvY;
        uint8_t normalX, normalY, normalZ, normalW;
        uint8_t tangentX, tangentY, tangentZ, tangentW;
        uint32_t padding0;
    };
    static_assert(sizeof(Vertex) % 16 == 0);

    struct alignas(16) HlslVtx
    {
        BlitML::vec3 position;
        float mappingU, mappingV;
        uint32_t normals, tangents;
        uint32_t padding0;
    };
    static_assert(sizeof(HlslVtx) % 16 == 0);

    struct alignas(16) Cluster
    {
        // Bounding sphere
    	BlitML::vec3 center;
    	float radius;

        // This is for backface culling
    	int8_t coneAxisX;
        int8_t coneAxisY;
        int8_t coneAxisZ;
    	int8_t coneCutoff;

        // Offset into the cluster data buffer (index buffer for clusters)
    	uint32_t dataOffset;

        // I am not sure why I have vertex count AND triangle count but whatever
    	uint8_t vertexCount;
    	uint8_t triangleCount;
        uint8_t padding0;
        uint8_t padding1;
    };

    struct alignas(16) Material
    {
        uint32_t albedoTag;
        uint32_t normalTag;
        uint32_t specularTag;
        uint32_t emissiveTag;

        uint32_t materialId;
        uint32_t padding0;
        uint32_t padding1;
        uint32_t padding2;
    };

    struct alignas(16) LodData
    {
        // Non cluster path, used to create draw commands
        uint32_t indexCount;
        uint32_t firstIndex;

		// Cluster path, used to create draw commands
        uint32_t clusterOffset;
        uint32_t clusterCount;

        // Used for more accurate LOD selection
        float error;

        // Pad to 32 bytes total 
        uint32_t padding0;  
        uint32_t padding1; 
        uint32_t padding2;
    };

    struct alignas(16) PrimitiveSurface
    {
        // Bounding sphere
        BlitML::vec3 center;     
        float radius;            

        uint32_t materialId;

        // Index to lod buffer
        // Each primitive can be drawn in multiple ways, depending on its LODs
        // The LOD is selected in compute shaders
        uint32_t lodOffset;
        uint32_t lodCount = 0;

        uint32_t vertexOffset; // Not used in the shaders but can hold the offset when loading
    };

    // A mesh is a collection of one or more primitives.
    struct Mesh
    {
        uint32_t firstSurface;
        uint32_t surfaceCount = 0;

        uint32_t meshId;
    };

    // Each mesh has a different transform. Passed to the GPU. Accessed through render object
    struct alignas(16) MeshTransform
    {
        BlitML::vec3 pos;
        float scale;
        BlitML::quat orientation;
    };

    // Per render object Data. Passed to the GPU
    struct RenderObject
    {
        uint32_t transformId;
        uint32_t surfaceId;
    };
}