#include "blitResourceRPF.h"
#include "Renderer/Resources/blitShaderResources.h"

namespace BlitzenEngine
{
	size_t GetRpfMeshSize(UPLOAD_MESH_RPF_CONTEXT& rpfCtx)
	{
        size_t writeSize = rpfCtx.m_idxCount * sizeof(uint32_t) + rpfCtx.m_meshPrimitiveCount * sizeof(PrimitiveSurface) + rpfCtx.m_lodCount * sizeof(LodData)
            + rpfCtx.m_vtxCount * sizeof(VtxPos) + rpfCtx.m_vtxCount * sizeof(VtxNormals) + rpfCtx.m_vtxCount * sizeof(VtxTangents) + rpfCtx.m_vtxCount * sizeof(VtxTexCoords) +
            GcRapidMeshFileHeaderSize + CE_BLITZEN_RAPID_MESH_FILE_PADDING_SIZE;// Finally adds header size and padding
        // Adds cluster if they are requested for this mesh
        if (rpfCtx.m_clustersBuiltFlag)
        {
            writeSize += rpfCtx.m_clusterCount * sizeof(ClusterVertices) + rpfCtx.m_clusterCount * sizeof(ClusterSphere) + rpfCtx.m_clusterCount * sizeof(ClusterCone) + rpfCtx.m_idxCount * sizeof(uint32_t);
        }

        if (writeSize > GcRapidMeshFileSizeThreshold)
        {
            return CE_GET_RPF_MESH_SIZE_ERROR_RETURN_CODE;
        }

        return writeSize;
	}

    const char* GetRpfMeshPath(const char* meshName, BlitCL::String& stringContainer)
    {
        if (stringContainer.GetSize() != 0) stringContainer.Clear();
        
        stringContainer.CopyString(GCRapidMeshDirectoryPath);

        char* filename = const_cast<char*>(meshName);
        char* extension = const_cast<char*>(GcRapidMeshFileExtension);
        stringContainer.Append(filename);
        stringContainer.Append(extension);

        return stringContainer.GetClassic();
    }
}