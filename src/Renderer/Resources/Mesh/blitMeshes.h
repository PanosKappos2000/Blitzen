#pragma once
#include "Renderer/Resources/blitShaderResources.h"
#include "BlitCL/blitHashMap.h"
#include "blitMeshPrimitive.h"
#include "blitTriangle.h"
#include "blitClusters.h"
#include "Renderer/Resources/RapidFile/blitResourceRPF.h"

namespace BlitzenEngine
{
    class MeshResources
    {
    public:

        Mesh m_meshes[BlitzenCore::Ce_MaxMeshCount];
        BlitCL::HashMap<Mesh> m_meshMap;
        uint32_t m_meshCount = 0;

        MeshPrimitivesContainer m_meshPrimitives{};
        PrimitiveContainer m_triangles{};
        ClusterContainer m_clusters{};

        uint32_t AddMesh(uint32_t firstSurface, uint32_t surfaceCount, const char* meshName = "BLIT_DO_NOT_ADD_TO_MESH_TABLE");
    };

    // Loads mesh from mesh obj. Returns its id or Ce_MaxMeshCount if error occurs
    uint32_t LoadMeshFromObj(MeshResources& context, const char* filename, const char* meshName);

    uint32_t LoadObjFileMeshToDisk(MeshResources& context, const char* filename, const char* meshName);

    uint32_t LoadMeshFromDisk();

    enum class UPLOAD_MESH_TO_DISK_RES : int64_t
    {
        SUCCESS = BlitzenCore::CE_BLITZEN_SUCCESS,
        FATAL = BlitzenCore::CE_BLITZEN_FATAL,
        FAILED_TO_OPEN_FILE = BlitzenCore::CE_BLITZEN_FATAL - 100,

        UPLOAD_SIZE_TOO_BIG = -1,
        FAILED_TO_UPLOAD_MESH_PRIMITIVES = -2,
        FAILED_TO_UPLOAD_MESH_PRIMITIVE_DATA = -3,
        FAILED_TO_UPLOAD_LOD_DATA = -4,
        FAILED_TO_UPLOAD_BOUNDING_SPHERE_DATA = -5,
        FAILED_TO_UPLOAD_VERTEX_POSITIONS_DATA = -6,
        FAILED_TO_UPLOAD_VERTEX_NORMALS_DATA = -7,
        FAILED_TO_UPLOAD_VERTEX_TANGENTS_DATA = -8,
        FAILED_TO_UPLOAD_VERTEX_TEXTURE_COORDINATES_DATA = -9,
        FAILED_TO_UPLOAD_VERTEX_INDICES = -10,
        FAILED_TO_UPLOAD_CLUSTER_VERTEX_DATA = -11,
        FAILED_TO_UPLOAD_CLUSTER_SPHERES = -12,
        FAILED_TO_UPLOAD_CLUSTER_CONES = -13,
        FAILED_TO_UPLOAD_CLUSTER_INDICES = -14,
        FAILED_TO_UPLOAD_OFFSETS_TO_HEADER = -15
    };
    UPLOAD_MESH_TO_DISK_RES UploadMeshToDisk(const char* meshName, BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE& memoryMappedFile, UPLOAD_MESH_RPF_CONTEXT& rpfCtx);

    void InitializeMeshResourcesPointer_STATIC_ACCESS(MeshResources* ptr);

	Mesh& RequestMeshResources_STATIC_ACCESS(const char* meshName);

    BoundingSphere* GetBoundingSphereResources_STATIC_ACCESS(Mesh* pMesh);

    BlitzenCore::FAT_BOOL GetMeshPrimitiveTransparencyFlag_STATIC_ACCESS(uint32_t surfaceID);

    inline const char* UPLOAD_MESH_TO_DISK_RES_TO_STRING(UPLOAD_MESH_TO_DISK_RES res)
    {
        switch (res)
        {
        case UPLOAD_MESH_TO_DISK_RES::SUCCESS: return "UPLOAD_MESH_TO_DISK_RES::SUCCESS";
        case UPLOAD_MESH_TO_DISK_RES::FATAL: return "UPLOAD_MESH_TO_DISK_RES::FATAL";
        case UPLOAD_MESH_TO_DISK_RES::FAILED_TO_OPEN_FILE: return "UPLOAD_MESH_TO_DISK_RES::FAILED_TO_OPEN_FILE";
        case UPLOAD_MESH_TO_DISK_RES::UPLOAD_SIZE_TOO_BIG: return "UPLOAD_MESH_TO_DISK_RES::UPLOAD_SIZE_TOO_BIG";
        case UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_MESH_PRIMITIVES: return "UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_MESH_PRIMITIVES";
        case UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_MESH_PRIMITIVE_DATA: return "UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_MESH_PRIMITIVE_DATA";
        case UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_LOD_DATA: return "UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_LOD_DATA";
        case UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_BOUNDING_SPHERE_DATA: return "UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_BOUNDING_SPHERE_DATA";
        case UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_VERTEX_POSITIONS_DATA: return "UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_VERTEX_POSITIONS_DATA";
        case UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_VERTEX_NORMALS_DATA: return "UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_VERTEX_NORMALS_DATA";
        case UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_VERTEX_TANGENTS_DATA: return "UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_VERTEX_TANGENTS_DATA";
        case UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_VERTEX_TEXTURE_COORDINATES_DATA: return "UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_VERTEX_TEXTURE_COORDINATES_DATA";
        case UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_VERTEX_INDICES: return "UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_VERTEX_INDICES";
        case UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_CLUSTER_VERTEX_DATA: return "UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_CLUSTER_VERTEX_DATA";
        case UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_CLUSTER_SPHERES: return "UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_CLUSTER_SPHERES";
        case UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_CLUSTER_CONES: return "UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_CLUSTER_CONES";
        case UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_CLUSTER_INDICES: return "UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_CLUSTER_INDICES";
        case UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_OFFSETS_TO_HEADER: return "UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_OFFSETS_TO_HEADER";
        default: return "UPLOAD_MESH_TO_DISK_RES::UNKNOWN";
        }
    }
}