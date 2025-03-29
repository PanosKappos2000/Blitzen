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
        constexpr uint8_t ce_buildClusters = 1;
    #else
        constexpr uint8_t ce_buildClusters = 0;
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
        // Vertex position (3 floats)
        BlitML::vec3 position;
        // Uv maps (2 halfs floats)
        uint16_t uvX, uvY;
        // Vertex normals (4 8-bit integers)
        uint8_t normalX, normalY, normalZ, normalW;
        // Tangents (4 8-bit integers)
        uint8_t tangentX, tangentY, tangentZ, tangentW;
    };

    // Data for each meshlet. Passed to the GPU
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

	// Data for each material. Passed to the GPU
    struct alignas(16) Material
    {
        // Not using these 2 right now, might remove them when I'm not bored, but they (or something similar) will be used in the future
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
    struct MeshLod
    {
        // Accesses index data
        uint32_t indexCount;
        uint32_t firstIndex;

        // Accesses meshlet data
        uint32_t meshletCount = 0;
        uint32_t firstMeshlet;

        // LOD error
        float error;
    };

    // Per primitive data. Passed to the GPU
    struct alignas(16) PrimitiveSurface
    {
        // Bounding sphere data, needed for frustum culling
        BlitML::vec3 center;
        float radius;

        MeshLod meshLod[ce_primitiveSurfaceMaxLODCount];// This is wasteful
        uint8_t lodCount = 0;

        uint32_t vertexOffset;

        uint32_t materialId;

        uint8_t postPass = 0;
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