#pragma once
#include "Core/blitzenEngine.h"
#include "BlitzenMathLibrary/blitMLTypes.h"

namespace BlitzenEngine
{
    using Resident = uint32_t;

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
        uint32_t clusterOffset;
        uint32_t clusterCount;
        float error;
        // Pad to 32 bytes total
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
        uint32_t materialId;
        uint32_t lodOffset;
        uint32_t lodCount{ 0 };
        uint32_t padding0;
    };

    struct MeshPrimitiveData
    {
        BlitzenCore::BIG_BOOL m_primitiveTransparencyFlags{ BlitzenCore::BB_FALSE };
        uint32_t m_primitiveVertexCount{ 0 };
        uint32_t m_primitiveVertexOffset{ UINT32_MAX };
    };

    struct alignas(16) MeshTransform
    {
        BlitML::vec3 pos;
        float scale;
        BlitML::quat orientation;
    };

    struct WVTransform
    {
        BlitML::vec3 position;
        BlitML::vec3 eulerAngles;
        uint32_t movementFlags = 0;
        uint32_t targetIdx;
    };

    struct RenderObject
    {
        uint32_t transformId;
        uint32_t surfaceId;
    };

    struct InstancedRenders
    {
        uint32_t surfaceID;
        uint32_t drawCmdID;
        uint32_t renderOffset;
    };

    struct BoundingSphere
    {
        BlitML::vec3 m_center;
        float m_radius;
    };
    static_assert(sizeof(BoundingSphere) % 16 == 0);

    struct AABB
    {
        BlitML::vec3 m_minBounds;
        BlitML::vec3 m_maxBounds;
        BlitML::float2 m_padding;
    };
    static_assert(sizeof(AABB) % 16 == 0);

    struct AABB_NOALIGN
    {
        BlitML::vec3 m_minBounds;
        BlitML::vec3 m_maxBounds;
    };

    struct WVGravity
    {
        float currentSpeed = 0.f;
        float maxSpeed = 1.f;
    };

    struct WVVelocity
    {
        float acceleration;
        float deceleration;
        float currentSpeed = 0.f;
        float maxSpeed;
    };

    using EulerAngles = BlitML::vec4;// padded

    struct GridCellOffsets
    {
        uint32_t colliderOffset;
        uint32_t colliderCount;
    };

    //---------------------------------------------------------------------------------------------------------------------------
    // FULL COLLIDER SPLIT INTO 2 STRUCTS
    // 
    // This design is mostly for compatibility and optimal use of SIMD
    // If I see that SIMD has no place in collision I will simplify this
    // 
    // The biggest weakness is the collider type saved as a float which requires casting every time that it's used
    //----------------------------------------------------------------------------------------------------------------------------
    struct ColliderAMaxRad
    {
        // First three components: Supposed Union for Capsule A and AABB Max. 
        // Last float component (w): Supposed Union for Capsule Radius and Sphere Radius
        // Not using true unions because this is a shader struct
        BlitML::float4 data; 
    };
    struct ColliderBMinType
    {
        // First three components: Supposed Union for Capsule B and AABB Min.
        // Last float component (w): Holds collider type (treated as uint32 or ColliderType)
        // Not using true unions because this is a shader struct
        BlitML::float4 data; 
    };

    struct SplitColliderDataPair
    {
        ColliderAMaxRad AMaxRad;
        ColliderBMinType BMinType;
    };

    struct BMPR_NARROW_PHASE_DRIVER
    {
        uint32_t cellIndex;
        uint32_t computeShaderGroupX;
        uint32_t computeShaderGroupY;
        uint32_t computeShaderGroupZ;
    };

    struct CollisionMessage
    {
        Resident m_impactingObject;
        Resident m_reactingResident;
    };
}