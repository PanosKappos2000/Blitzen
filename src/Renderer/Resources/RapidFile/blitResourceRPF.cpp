#include "blitResourceRPF.h"
#include "Renderer/Resources/blitShaderResources.h"

namespace BlitzenEngine
{
	size_t GetRpfMeshSize(MeshResources& context, bool clustersBuildFlag)
	{
        constexpr uint32_t LCMeshPrimitiveCountPerRpfMeshFile = 1;
        size_t writeSize = LCMeshPrimitiveCountPerRpfMeshFile * sizeof(PrimitiveSurface) + LCMeshPrimitiveCountPerRpfMeshFile * sizeof(BoundingSphere) 
			+ LCMeshPrimitiveCountPerRpfMeshFile * sizeof(MeshPrimitiveData) + LCMeshPrimitiveCountPerRpfMeshFile * sizeof(ColliderAMaxRad) + LCMeshPrimitiveCountPerRpfMeshFile * sizeof(ColliderBMinType) +
            + context.m_meshPrimitives.m_LODCount * sizeof(LodData) + context.m_triangles.m_vertexCount * sizeof(VtxPos) + context.m_triangles.m_vertexCount * sizeof(VtxNormals) 
            + context.m_triangles.m_vertexCount * sizeof(VtxTangents) + context.m_triangles.m_vertexCount * sizeof(VtxTexCoords) + context.m_triangles.m_vtxIdxCount * sizeof(uint32_t) 
            + GcRapidMeshFileHeaderSize + CE_BLITZEN_RAPID_MESH_FILE_PADDING_SIZE;// Finally adds header size and padding
        // Adds cluster if they are requested for this mesh
        if (clustersBuildFlag)
        {
            writeSize += context.m_clusters.m_clusterCount * sizeof(ClusterVertices) + context.m_clusters.m_clusterCount * sizeof(ClusterSphere) + context.m_clusters.m_clusterCount * sizeof(ClusterCone) + 
                context.m_clusters.m_clusterCount * sizeof(uint32_t);
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