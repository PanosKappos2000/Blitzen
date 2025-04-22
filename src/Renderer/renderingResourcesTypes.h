#include "BlitzenMathLibrary/blitML.h"
#include <string>

namespace BlitzenEngine
{
    constexpr uint32_t ce_maxTextureCount = 5'000;

    constexpr uint32_t ce_maxMaterialCount = 10'000;

    constexpr uint8_t ce_primitiveSurfaceMaxLODCount = 8;

    constexpr uint32_t ce_maxMeshCount = 1'000'000; 
	constexpr const char* ce_defaultMeshName = "bunny";

    constexpr uint32_t ce_maxRenderObjects = 5'000'000;

    constexpr uint32_t ce_maxONPC_Objects = 100;

    #ifdef BLITZEN_CLUSTER_CULLING
        constexpr uint8_t Ce_BuildClusters = 1;
    #else
        constexpr uint8_t Ce_BuildClusters = 0;
    #endif 
    
    struct TextureStats
    {
		std::string filepath;// Probably useless, but whatever
        uint8_t* pTextureData;// Probably useless, but whatever
        uint32_t textureTag;// Do I even use this piece of crap?
    };

    struct alignas(16) Vertex
    {
        BlitML::vec3 position;
        uint16_t uvX, uvY;
        uint8_t normalX, normalY, normalZ, normalW;
        uint8_t tangentX, tangentY, tangentZ, tangentW;
    };

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
        // Not using these 2 right now, might remove them when I'm not bored, 
        // but they (or something similar) will be used in the future
        BlitML::vec4 diffuseColor;
        float shininess;

        uint32_t albedoTag;// Index into the texture array, for the albedo map of the material

        uint32_t normalTag;// Index into the texture array, for the normal map of the material

        uint32_t specularTag;// Index into the texture array, for the specular map of the material

        uint32_t emissiveTag;// Index into the texture array for the emissive map of the material

        // TODO: I need to try removing this, it's a waste of space
        uint32_t materialId;
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
        uint32_t padding0; // Explicit padding to 48 bytes total
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