#include "blitRenderingResources.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenEngine
{
    inline RenderingResources* GSRenderingResources = nullptr;

    bool CopyMeshResourcesToStagingBuffer(MeshResources* pMeshes, RenderingLoadingContextMesh& loadingContextMesh)
    {
        if (!UploadToVertexPositionsStagingBuffer(loadingContextMesh, pMeshes->m_triangles.m_vertexPositions, pMeshes->m_triangles.m_vertexCount))
        {
            BLIT_ERROR("%s: Failed to upload vertex positions to staging buffer", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        if (!UploadToVertexNormalsStagingBuffer(loadingContextMesh, pMeshes->m_triangles.m_vertexNormals, pMeshes->m_triangles.m_vertexCount))
        {
            BLIT_ERROR("%s: Failed to upload vertex normals to staging buffer", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        if (!UploadToVertexTangentsStagingBuffer(loadingContextMesh, pMeshes->m_triangles.m_vertexTangents, pMeshes->m_triangles.m_vertexCount))
        {
            BLIT_ERROR("%s: Failed to upload vertex tangents to staging buffer", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        if (!UploadToVertexTextureCoordinatesStagingBuffer(loadingContextMesh, pMeshes->m_triangles.m_vertexUVs, pMeshes->m_triangles.m_vertexCount))
        {
            BLIT_ERROR("%s: Failed to upload vertex texture coordinates to staging buffer", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        // Offset indices for current index count. This is not optimal if it is to happen in flight. I could save the vertex offset on the mesh primitive struct
        for (uint32_t i = 0; i < pMeshes->m_triangles.m_vtxIdxCount; ++i)
        {
            pMeshes->m_triangles.m_indices[i] += pMeshes->m_triangles.m_mapVtxCount;
        }

        if (!UploadToVertexIndicesStagingBuffer(loadingContextMesh, pMeshes->m_triangles.m_indices, pMeshes->m_triangles.m_vtxIdxCount))
        {
            BLIT_ERROR("%s: Failed to upload vertex indices to staging buffer", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        for (uint32_t lod = 0; lod < pMeshes->m_meshPrimitives.m_LODCount; ++lod)
        {
            pMeshes->m_meshPrimitives.m_LODs[lod].firstIndex += pMeshes->m_triangles.m_mapIdxCount;
        }

        if (!UploadToLODDataStagingBuffer(loadingContextMesh, pMeshes->m_meshPrimitives.m_LODs, pMeshes->m_meshPrimitives.m_LODCount))
        {
            BLIT_ERROR("%s: Failed to upload Level of Detail data to staging buffer", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        pMeshes->m_meshPrimitives.m_meshPrimitives[pMeshes->m_meshPrimitives.m_meshPrimitivesCount - 1].lodOffset += pMeshes->m_meshPrimitives.mMapLodCount;

        if (!UploadToMeshPrimitiveStagingBuffer(loadingContextMesh, &pMeshes->m_meshPrimitives.m_meshPrimitives[pMeshes->m_meshPrimitives.m_meshPrimitivesCount - 1], 1))
        {
            BLIT_ERROR("%s: Failed to upload level of Detail data to staging buffer", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        pMeshes->UpdateMapMeshContext();
        pMeshes->ResetMeshContext();

        // success
        return true;
    }

    bool GetMaterialTransparencyFlag(uint32_t materialID)
    {
        BLIT_RUNTIME_TEST_CHECK_ASSERT(materialID < GSRenderingResources->mMaterials.mMaterialCount);

        return GSRenderingResources->mMaterials.mMatData[materialID].transparencyFlag != MaterialAlphaMode::Opaque;
    }

    void InitializeRenderingResourcesGlobalPointer(RenderingResources* ptr)
    {
        BLIT_ASSERT(GSRenderingResources == nullptr);

        GSRenderingResources = ptr;
    }
}