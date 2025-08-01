#include "blitResourceRPF.h"
#include "Renderer/Resources/blitShaderResources.h"
#include "Core/DbLog/blitLogger.h"

namespace BlitzenEngine
{
    bool UploadMaterialsToDisk(const char* folderName, Material* matArray, MaterialData* matDataArray, uint32_t count)
    {
        BlitCL::FatString filepath{ strlen(GCRapidMeshDirectoryPath) + strlen(folderName) + strlen("/") + strlen("matBatch.blitMat") };
        filepath.Format("%s%s/matBatch.blitMat", GCRapidMeshDirectoryPath, folderName);

        uint32_t headerWriteSize = GCMaterialHeaderElementCount * sizeof(size_t);
        uint32_t materialWriteSize = count * sizeof(Material);
        uint32_t materialDataWriteSize = count * sizeof(MaterialData);
        uint32_t writeSize = materialWriteSize + materialDataWriteSize + headerWriteSize;

        BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE materialFile;
        auto materialFileOpenRes = materialFile.OpenWrite(filepath.Get(), writeSize);
        if (BlitzenPlatform::CHECK_BLIT_MMF_RES_FOR_ERROR(materialFileOpenRes))
        {
            BLIT_ERROR("%s: Could not open rpf file for material batch write", BlitzenCore::GCRpfSystemName);
            return false;
        }

        BlitRpfMaterialHeader materialHeader{};

        uint32_t offset = headerWriteSize;
        if (!BlitzenPlatform::WriteMemoryMappedFile(materialFile, offset, materialWriteSize, matArray))
        {
            BLIT_ERROR("%s: Failed to write material texture indices to disk", BlitzenCore::GCRpfSystemName);
            return false;
        }
        materialHeader[BlitRpfMaterialOffsetID] = offset;
        offset += materialWriteSize;

        if (!BlitzenPlatform::WriteMemoryMappedFile(materialFile, offset, materialDataWriteSize, matDataArray))
        {
            BLIT_ERROR("%s: Failed to write material data to disk", BlitzenCore::GCRpfSystemName);
            return false;
        }
        materialHeader[BlitRpfMaterialDataOffsetID] = offset;
        offset += materialDataWriteSize;

        constexpr uint32_t LCStartOfFile = 0;
        if (!BlitzenPlatform::WriteMemoryMappedFile(materialFile, LCStartOfFile, headerWriteSize, materialHeader))
        {
            BLIT_ERROR("%s: Fialed to write material header to disk", BlitzenCore::GCRpfSystemName);
            return false;
        }

        return true;
    }

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

    const char* BuildImportedSceneNodesFilepath(const char* sceneName, BlitCL::String& stringContainer)
    {
        if (stringContainer.GetSize() != 0) stringContainer.Clear();

        stringContainer.CopyString(GCRapidMeshDirectoryPath);
        stringContainer.Append(const_cast<char*>(sceneName));
        stringContainer.Append("/");
        stringContainer.Append(const_cast<char*>(GCImportedSceneNodesFileName));

        return stringContainer.GetClassic();
    }
}