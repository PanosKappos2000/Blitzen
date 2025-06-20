#pragma once
#include "Core/blitzenEngine.h"
#include "BlitzenMathLibrary/blitMLTypes.h"

namespace BlitzenEngine
{
    struct Mesh
    {
        uint32_t firstSurface;
        uint32_t surfaceCount{ 0 };
        uint32_t meshId;
    };

    using VtxPos = BlitML::vec3;
    using VtxTexCoords = BlitML::vec2;
    using VtxNormals = BlitML::vec4;
    using VtxTangents = BlitML::vec4;

    struct alignas(16) Vertex
    {
        BlitML::vec3 position;
        float uvX, uvY;
        uint8_t normalX, normalY, normalZ, normalW;
        uint8_t tangentX, tangentY, tangentZ, tangentW;
        uint32_t padding0;
    };
    static_assert(sizeof(Vertex) % 16 == 0);

    struct alignas(16) Cluster
    {
    	BlitML::vec3 center;
    	float radius;
    	int8_t coneAxisX;
        int8_t coneAxisY;
        int8_t coneAxisZ;
    	int8_t coneCutoff;
    	uint32_t dataOffset;
    	uint8_t vertexCount;
    	uint8_t triangleCount;
        uint8_t padding0;
        uint8_t padding1;
    };

    struct ClusterVertices
    {
        uint32_t idxOffset;
        uint32_t idxCount;
    };
    
    struct ClusterSphere
    {
        BlitML::vec3 center;
        float radius;
    };
    
    struct ClusterCone
    {
        BlitML::vec3 cone;
        float coneCutoff;
    };

    struct alignas(16) LodData
    {
        uint32_t indexCount;
        uint32_t firstIndex;
        // Cluster path
        uint32_t clusterOffset;
        uint32_t clusterCount;
        float error;
        uint32_t padding0;  
        uint32_t padding1;
        uint32_t padding2;
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
    static_assert(sizeof(Material) % 16 == 0);

    struct alignas(16) PrimitiveSurface
    {
        // TODO: Should be owned by render instances, for static object optimizations
        BlitML::vec3 center;     
        float radius;
        /* THE ACTUAL MEMBERS AFTER THE CHANGE */
        uint32_t materialId;
        uint32_t lodOffset;
        uint32_t lodCount{ 0 };
        uint32_t padding0;
    };

    struct alignas(16) MeshTransform
    {
        BlitML::vec3 pos;
        float scale;
        BlitML::quat orientation;
    };

    struct RenderObject
    {
        uint32_t transformId;
        uint32_t surfaceId;
        // uint32_t staticBoundingSphere; TODO: Own bounding sphere
    };

    struct BoundingSphere
    {
        BlitML::vec3 m_center;
        float m_radius;
    };
    static_assert(sizeof(BoundingSphere) % 16 == 0);

    struct Velocity
    {
        BlitML::vec3 m_velocity;
    };

    struct Rotation
    {
        BlitML::fRotation m_rotation;
    };
}