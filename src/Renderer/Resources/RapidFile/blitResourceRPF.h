#pragma once
#include "Core/BlitzenEngine.h"
#include "Platform/Common/blitMappedFile.h"
#include "Renderer/Resources/blitShaderResources.h"

namespace BlitzenEngine
{
	constexpr const char* CE_BLITZEN_RAPID_MESH_FILE_EXTENSION = ".blitMesh";
	constexpr size_t CE_BLITZEN_RAPID_MESH_FILE_SIZE_THRESHOLD = 1024 * 1024 * 10; // 10 MB (Probably too much but oh well, it's just for checking)
    constexpr size_t CE_BLITZEN_RAPID_MESH_FILE_HEADER_SIZE = 1000;
    constexpr size_t CE_BLITZEN_RAPID_MESH_FILE_PADDING_SIZE = 1000;
    constexpr const char* CE_BLITZEN_RAPID_MESH_DIRECTORY_PATH = "Assets/BlitzenRapidMeshes/";

    // Straight enum for ease of use
    enum BLIT_RPF_MESH_FILE_HEADER_IDS
    {
        BLIT_RPF_MESH_FILE_HEADER_MESH_PRIMITIVE_ID = 0,
        BLIT_RPF_MESH_FILE_HEADER_MESH_PRIMITIVE_DATA_ID = 1,
        BLIT_RPF_MESH_FILE_HEADER_LOD_DATA_ID = 2,
        BLIT_RPF_MESH_FILE_HEADER_BOUNDING_SPHERE_ID = 3,
        BLIT_RPF_MESH_FILE_HEADER_VTX_POS_DATA_ID = 4,
        BLIT_RPF_MESH_FILE_HEADER_VTX_NRM_DATA_ID = 5,
        BLIT_RPF_MESH_FILE_HEADER_VTX_TNG_DATA_ID = 6,
        BLIT_RPF_MESH_FILE_HEADER_VTX_COORD_DATA_ID = 7,
        BLIT_RPF_MESH_FILE_HEADER_VTX_IDXS_DATA_ID = 8,
        BLIT_RPF_MESH_FILE_HEADER_CLUSTER_VTXS_DATA_ID = 9,
        BLIT_RPF_MESH_FILE_HEADER_CLUSTER_SPHERES_DATA_ID = 10,
        BLIT_RPF_MESH_FILE_HEADER_CLUSTER_CONES_DATA_ID = 11,
        BLIT_RPF_MESH_FILE_HEADER_CLUSTER_IDXS_DATA_ID = 12
    };
    constexpr uint32_t CE_BLIT_RPF_MESH_FILE_HEADER_ELEMENT_COUNT = 13;
    static_assert(CE_BLIT_RPF_MESH_FILE_HEADER_ELEMENT_COUNT * sizeof(size_t) < CE_BLITZEN_RAPID_MESH_FILE_HEADER_SIZE);

    using BLIT_RPF_MESH_FILE_HEADER_ARR = size_t[CE_BLIT_RPF_MESH_FILE_HEADER_ELEMENT_COUNT];

    struct UPLOAD_MESH_RPF_CONTEXT
    {
        Mesh* pMesh;
        PrimitiveSurface* m_surfaceArray;
        MeshPrimitiveData* m_meshPrimitiveData;
        uint32_t m_meshPrimitiveCount;
        LodData* m_lodDataArr;
        uint32_t m_lodCount;
        BoundingSphere* m_boundsArr;
        uint32_t* m_vtxIdxArr;
        uint32_t m_idxCount;
        uint32_t* m_clusterIdxArr;
        VtxPos* m_vtxPosArr;
        VtxNormals* m_vtxNormalsArr;
        VtxTangents* m_vtxTangArr;
        VtxTexCoords* m_vtxTexCoordArr;
        uint32_t m_vtxCount;
        bool m_clustersBuiltFlag;
        ClusterVertices* m_clusterVtxArr;
        ClusterSphere* m_clusterSphereArr;
        ClusterCone* m_clusterConeArr;
        uint32_t m_clusterCount;
    };

    constexpr size_t CE_GET_RPF_MESH_SIZE_ERROR_RETURN_CODE = CE_BLITZEN_RAPID_MESH_FILE_SIZE_THRESHOLD;
	size_t GetRpfMeshSize(UPLOAD_MESH_RPF_CONTEXT& rpfCtx);

    // FOR FUTURE REFERENCE
    struct MeshRpfOffsetAccessor
    {
        size_t m_offset;
        size_t m_count;
    };
    using BLIT_RPF_MESH_FILE_OFFSETS_ARR = MeshRpfOffsetAccessor[CE_BLIT_RPF_MESH_FILE_HEADER_ELEMENT_COUNT];

    constexpr const char* CE_RPF_MESH_CONTEXT_FILE_NAME = "Assets/BlitzenRapidMeshes/context.blitMesh";
    constexpr const char* CE_RPF_MESH_OFFSETS_FILE_NAME = "Assets/BlitzenRapidMeshes/offsets.blitMesh";
    constexpr const char* CE_RPF_MESH_ASSETS_FILE_NAME = "Assets/BlitzenRapidMeshes/assets.blitMesh";
    class MESH_RPF_OFFSET_MANAGER
    {
    public:
        BLIT_RPF_MESH_FILE_OFFSETS_ARR m_offsets;
        size_t offset;
        BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE m_offsetFile;
        BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE m_assetFile;

        void Startup();

        void CloseAndSave();

        // Might not be needed since the mmf already does destructor memory cleanup
        ~MESH_RPF_OFFSET_MANAGER();
    };
}