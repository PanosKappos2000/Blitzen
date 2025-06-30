#include "blitResourceRPF.h"
#include "Renderer/Resources/blitShaderResources.h"

namespace BlitzenEngine
{
	size_t GetRpfMeshSize(UPLOAD_MESH_RPF_CONTEXT& rpfCtx)
	{
        size_t writeSize = rpfCtx.m_idxCount * sizeof(uint32_t) + rpfCtx.m_meshPrimitiveCount * sizeof(PrimitiveSurface) + rpfCtx.m_lodCount * sizeof(LodData)
            + rpfCtx.m_vtxCount * sizeof(VtxPos) + rpfCtx.m_vtxCount * sizeof(VtxNormals) + rpfCtx.m_vtxCount * sizeof(VtxTangents) + rpfCtx.m_vtxCount * sizeof(VtxTexCoords) +
            CE_BLITZEN_RAPID_MESH_FILE_HEADER_SIZE + CE_BLITZEN_RAPID_MESH_FILE_PADDING_SIZE;
        if (rpfCtx.m_clustersBuiltFlag)
        {
            writeSize += rpfCtx.m_clusterCount * sizeof(ClusterVertices) + rpfCtx.m_clusterCount * sizeof(ClusterSphere) + rpfCtx.m_clusterCount * sizeof(ClusterCone) + rpfCtx.m_idxCount * sizeof(uint32_t);
        }

        if (writeSize > CE_BLITZEN_RAPID_MESH_FILE_SIZE_THRESHOLD)
        {
            return CE_GET_RPF_MESH_SIZE_ERROR_RETURN_CODE;
        }

        return writeSize;
	}
}