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
    
    // Data for each texture
    struct TextureStats
    {
        std::string filepath;

        // TODO: this is no longer used, might want to remove later
        uint8_t* pTextureData;

        // This tag is for the shaders to know which image memory to access, when a material uses this texture
        uint32_t textureTag;
    };

    // Data for each vertex. Passed to the GPU
    struct alignas(16) Vertex
    {
        BlitML::vec3 position;
        uint16_t uvX, uvY;
        uint8_t normalX, normalY, normalZ, normalW;
        uint8_t tangentX, tangentY, tangentZ, tangentW;
    };

    // Data for each meshlet. Passed to the GPU
    struct alignas(16) Cluster
    {
        // Bounding sphere for frustum culling
    	BlitML::vec3 center;
    	float radius;

        // This is for backface culling
    	int8_t coneAxisX;
        int8_t coneAxisY;
        int8_t coneAxisZ;
    	int8_t coneCutoff;

    	uint32_t dataOffset; // Index into meshlet data
    	uint8_t vertexCount;
    	uint8_t triangleCount;
        uint8_t padding0;
        uint8_t padding1;

        uint32_t gpuAlignmentPad;
    };

	// Data for each material. Passed to the GPU
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

    // Mesh LOD data. An array of these is held by each primitive
    struct LodData
    {
        uint32_t indexCount;
        uint32_t firstIndex;
        float error;

        uint32_t firstMeshlet;
        uint32_t meshletCount;

        uint32_t padding0; // Pad to 24 bytes total   
    };

    // Per primitive data. Passed to the GPU
    struct alignas(16) PrimitiveSurface
    {
        BlitML::vec3 center;     
        float radius;            

        uint32_t vertexOffset;

        uint32_t materialId;

        uint32_t lodCount = 0;       
        uint32_t padding0;      
        LodData meshLod[ce_primitiveSurfaceMaxLODCount];
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