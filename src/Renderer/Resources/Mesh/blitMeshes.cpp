#include "blitMeshes.h"
#include "BlitCL/blitDynamicArr.h"
// https://github.com/thisistherk/fast_obj
#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"
#include "objparser.h"
// Algorithms for building meshlets, loading LODs, optimizing vertex caches etc.
// https://github.com/zeux/meshoptimizer
#include "Meshoptimizer/meshoptimizer.h"
#include "BlitzenMathLibrary/blitML.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"
#include "Renderer/Resources/RapidFile/blitResourceRPF.h"

namespace BlitzenEngine
{
    inline MeshResources* GSMeshResources = nullptr;

    void InitializeMeshResourcesPointer_STATIC_ACCESS(MeshResources* ptr)
    {
        BLIT_ASSERT_MESSAGE(GSMeshResources == nullptr, "Attempted to reinitialize mesh resources singleton pointer");

        GSMeshResources = ptr;
    }

    BoundingSphere* GetVisibilityBoundingSphereFromMesh(Mesh* pMesh)
    {
        return &GSMeshResources->m_meshPrimitives.m_boundingSpheres[pMesh->firstSurface];
    }

    BoundingSphere& GetVisibilityBoundingSphereFromMeshPrimitive(uint32_t resourceID)
    {
        return GSMeshResources->m_meshPrimitives.m_boundingSpheres[resourceID];
    }

    SplitColliderDataPair& GetColliderFromMeshPrimitive(uint32_t resourceID)
    {
        return GSMeshResources->m_meshPrimitives.mColliders[resourceID];
    }

    Mesh& RequestMeshResources_STATIC_ACCESS(const char* meshName)
    {
		return GSMeshResources->m_meshMap[meshName];
    }

    BlitzenCore::FAT_BOOL GetMeshPrimitiveTransparencyFlag_STATIC_ACCESS(uint32_t surfaceID)
    {
        return GSMeshResources->m_meshPrimitives.m_meshPrimitiveData[surfaceID].m_primitiveTransparencyFlags;
    }

    uint32_t MeshResources::AddMesh(uint32_t firstSurface, uint32_t surfaceCount, const char* meshName /*="BLIT_DO_NOT_ADD_TO_MESH_TABLE"*/)
    {
        if (m_meshCount >= BlitzenCore::Ce_MaxMeshCount)
        {
            BLIT_ERROR("%s: Max mesh count: ( %i ) reached!", BlitzenCore::CE_MESH_SYSTEM_NAME, BlitzenCore::Ce_MaxMeshCount);
            return BlitzenCore::Ce_MaxMeshCount;
        }

        auto& mesh = m_meshes[m_meshCount];

        mesh.firstSurface = firstSurface;
        mesh.surfaceCount = surfaceCount;
        mesh.meshId = uint32_t(m_meshCount);

		if (meshName != "BLIT_DO_NOT_ADD_TO_MESH_TABLE")
		{
			m_meshMap.Insert(meshName, mesh);
		}

        return m_meshCount++;
    }

    void MeshResources::UpdateMapMeshContext()
    {
        m_meshPrimitives.mMapLodCount += m_meshPrimitives.m_LODCount;
        m_triangles.m_mapVtxCount += m_triangles.m_vertexCount;
        m_triangles.m_mapIdxCount += m_triangles.m_vtxIdxCount;
    }

    void MeshResources::ResetMeshContext()
    {
        // Restart mesh vertices but save mesh vertex count for offsets
        m_triangles.m_vtxIdxCount = 0;
        m_triangles.m_vertexCount = 0;
        for (uint32_t lod = 0; lod < m_meshPrimitives.m_LODCount; ++lod)
        {
            m_meshPrimitives.m_LODs[lod].indexCount = 0;
        }
        m_meshPrimitives.m_LODCount = 0;
        m_meshPrimitives.m_meshPrimitives[0].lodCount = 0;
    }

    uint32_t LoadMeshFromObj(MeshResources& context, const char* filename, const char* meshName)
    {
        // The function should return if the engine will go over the max allowed mesh assets
        if (context.m_meshCount >= BlitzenCore::Ce_MaxMeshCount)
        {
            BLIT_ERROR("%s: Max mesh count: ( %i ) reached!", BlitzenCore::CE_MESH_SYSTEM_NAME, BlitzenCore::Ce_MaxMeshCount);
            return BlitzenCore::Ce_MaxMeshCount;
        }

        // Get the current mesh and give it the size surface array as its first surface index
        uint32_t previousSurfaceCount{ context.m_meshPrimitives.m_meshPrimitivesCount };

        ObjFile file;
        if (!objParseFile(file, filename))
        {
            BLIT_ERROR("%s: Failed to parse obj file", BlitzenCore::CE_MESH_SYSTEM_NAME);
            return BlitzenCore::Ce_MaxMeshCount;
        }

        if (file.f_size * 3 > GCMaxVertexIndicesInMeshResource)
        {
            BLIT_ERROR("%s: Obj model holds too many possible indices")
            return BlitzenCore::Ce_MaxMeshCount;
        }

        uint32_t indexCount = (uint32_t)file.f_size / 3;

        BlitCL::DynamicArray<Vertex> triangleVertices{ indexCount };

        for (uint32_t i = 0; i < indexCount; ++i)
        {
            auto& vtx = triangleVertices[i];

            int32_t vertexIndex = file.f[i * 3 + 0];
            int32_t vertexTextureIndex = file.f[i * 3 + 1];
            int32_t vertexNormalIndex = file.f[i * 3 + 2];

            vtx.position.x = file.v[vertexIndex * 3 + 0];
            vtx.position.y = file.v[vertexIndex * 3 + 1];
            vtx.position.z = file.v[vertexIndex * 3 + 2];

            // Load the normal and turn them to 8 bit integers
            float normalX = vertexNormalIndex < 0 ? 0.f : file.vn[vertexNormalIndex * 3 + 0];
            float normalY = vertexNormalIndex < 0 ? 0.f : file.vn[vertexNormalIndex * 3 + 1];
            float normalZ = vertexNormalIndex < 0 ? 1.f : file.vn[vertexNormalIndex * 3 + 2];

            vtx.normalX = uint8_t(normalX * 127.f + 127.5f);
            vtx.normalY = uint8_t(normalY * 127.f + 127.5f);
            vtx.normalZ = uint8_t(normalZ * 127.f + 127.5f);

            vtx.tangentX = vtx.tangentY = vtx.tangentZ = 127;
            vtx.tangentW = 254;

            vtx.uvX = vertexTextureIndex < 0 ? 0.f : file.vt[vertexTextureIndex * 3 + 0];
            vtx.uvY = vertexTextureIndex < 0 ? 0.f : file.vt[vertexTextureIndex * 3 + 1];
        }

        // Creates indices for the obj's vertices using meshopt
        BlitCL::DynamicArray<uint32_t> remap(indexCount);
        size_t vertexCount = meshopt_generateVertexRemap(remap.Data(), 0, indexCount, triangleVertices.Data(), indexCount, sizeof(Vertex));
        BlitCL::DynamicArray<uint32_t> indices(indexCount);
        BlitCL::DynamicArray<Vertex> vertices(vertexCount);
        meshopt_remapVertexBuffer(vertices.Data(), triangleVertices.Data(), indexCount, sizeof(Vertex), remap.Data());
        meshopt_remapIndexBuffer(indices.Data(), 0, indexCount, remap.Data());

        MESH_PRIMITIVE_CREATE_CONTEXT primitiveCreateCtx{};
        primitiveCreateCtx.m_indexCount = (uint32_t)indices.GetSize();
        primitiveCreateCtx.m_indices = indices.Data();
        primitiveCreateCtx.m_vertexCount = (uint32_t)vertices.GetSize();
        primitiveCreateCtx.m_vertices = vertices.Data();
        auto meshPrimitiveGenRes{ context.m_meshPrimitives.GenerateSurface(context.m_triangles, context.m_clusters, primitiveCreateCtx) };
        if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)meshPrimitiveGenRes))
        {
            BlitzenCore::LOG_ERROR_MSG_AND_RETURN(BlitzenCore::CE_MESH_SYSTEM_NAME, MESH_PRIMITIVE_CREATE_RES_TO_STRING(meshPrimitiveGenRes));
            return BlitzenCore::Ce_MaxMeshCount;
        }

        context.m_meshPrimitives.GenerateTangents(primitiveCreateCtx);

        uint32_t meshId = context.AddMesh(previousSurfaceCount, uint32_t(context.m_meshPrimitives.m_meshPrimitivesCount - previousSurfaceCount), meshName);
        if (meshId == BlitzenCore::Ce_MaxMeshCount)
        {
            BLIT_ERROR("%s: Retrieved error count from AddMesh function", BlitzenCore::CE_MESH_SYSTEM_NAME);
            return BlitzenCore::Ce_MaxMeshCount;
        }

        return meshId;
    }

    bool LoadObjFileMeshToDisk(MeshResources& context, const char* filename, const char* meshName)
    {
        ObjFile file;
        if (!objParseFile(file, filename))
        {
            BLIT_ERROR("%s: Failed to parse obj file", BlitzenCore::CE_MESH_SYSTEM_NAME);
            return false;
        }

        if (file.f_size * 3 > GCMaxVertexIndicesInMeshResource)
        {
            BLIT_ERROR("%s: Obj model holds too many possible indices");
            return false;
        }

        uint32_t indexCount = (uint32_t)file.f_size / 3;

        BlitCL::DynamicArray<Vertex> triangleVertices{ indexCount };

        for (uint32_t i = 0; i < indexCount; ++i)
        {
            auto& vtx = triangleVertices[i];

            int32_t vertexIndex = file.f[i * 3 + 0];
            int32_t vertexTextureIndex = file.f[i * 3 + 1];
            int32_t vertexNormalIndex = file.f[i * 3 + 2];

            vtx.position.x = file.v[vertexIndex * 3 + 0];
            vtx.position.y = file.v[vertexIndex * 3 + 1];
            vtx.position.z = file.v[vertexIndex * 3 + 2];

            // Load the normal and turn them to 8 bit integers
            float normalX = vertexNormalIndex < 0 ? 0.f : file.vn[vertexNormalIndex * 3 + 0];
            float normalY = vertexNormalIndex < 0 ? 0.f : file.vn[vertexNormalIndex * 3 + 1];
            float normalZ = vertexNormalIndex < 0 ? 1.f : file.vn[vertexNormalIndex * 3 + 2];

            vtx.normalX = uint8_t(normalX * 127.f + 127.5f);
            vtx.normalY = uint8_t(normalY * 127.f + 127.5f);
            vtx.normalZ = uint8_t(normalZ * 127.f + 127.5f);

            vtx.tangentX = vtx.tangentY = vtx.tangentZ = 127;
            vtx.tangentW = 254;

            vtx.uvX = vertexTextureIndex < 0 ? 0.f : file.vt[vertexTextureIndex * 3 + 0];
            vtx.uvY = vertexTextureIndex < 0 ? 0.f : file.vt[vertexTextureIndex * 3 + 1];
        }

        // Creates indices for the obj's vertices using meshopt
        BlitCL::DynamicArray<uint32_t> remap(indexCount);
        size_t vertexCount = meshopt_generateVertexRemap(remap.Data(), 0, indexCount, triangleVertices.Data(), indexCount, sizeof(Vertex));
        BlitCL::DynamicArray<uint32_t> indices(indexCount);
        BlitCL::DynamicArray<Vertex> vertices(vertexCount);
        meshopt_remapVertexBuffer(vertices.Data(), triangleVertices.Data(), indexCount, sizeof(Vertex), remap.Data());
        meshopt_remapIndexBuffer(indices.Data(), 0, indexCount, remap.Data());

        // I am slowly fully converting to this format. The main block is meshoptmizer
        HLSL_VTX_CONTEXT hlslVerticesContext{};
        hlslVerticesContext.m_vtxPosArr = context.m_triangles.m_vertexPositions;
        hlslVerticesContext.m_vtxNrmArr = context.m_triangles.m_vertexNormals;
        hlslVerticesContext.m_vtxTngArr = context.m_triangles.m_vertexTangents;
        hlslVerticesContext.m_texCoordArr = context.m_triangles.m_vertexUVs;
        ConvertClassicVerticesToHlslFormat(hlslVerticesContext, vertices.Data(), (uint32_t)vertexCount);

        MESH_PRIMITIVE_CREATE_CONTEXT primitiveCreateCtx{};
        primitiveCreateCtx.m_indexCount = (uint32_t)indices.GetSize();
        primitiveCreateCtx.m_indices = indices.Data();
        primitiveCreateCtx.m_vertexCount = (uint32_t)vertices.GetSize();
        primitiveCreateCtx.m_vertices = vertices.Data();
        auto meshPrimitiveGenRes{ context.m_meshPrimitives.GenerateSurface(context.m_triangles, context.m_clusters, primitiveCreateCtx) };
        if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)meshPrimitiveGenRes))
        {
            BlitzenCore::LOG_ERROR_MSG_AND_RETURN(BlitzenCore::CE_MESH_SYSTEM_NAME, MESH_PRIMITIVE_CREATE_RES_TO_STRING(meshPrimitiveGenRes));
            return false;
        }

        uint32_t meshID = context.m_meshCount;

		BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE memoryMappedFile;
        auto uploadRes{ UploadMeshToDisk(meshName, memoryMappedFile, context, false) };
        BLIT_ASSERT_MESSAGE(!BlitzenCore::BLIT_CHECK_FATAL(int64_t(uploadRes)), UPLOAD_MESH_TO_DISK_RES_TO_STRING(uploadRes))
        if(BlitzenCore::BLIT_CHECK_FAIL(int64_t(uploadRes)))
        {
            BLIT_ERROR("%s: Failed to upload mesh resource to disk. Received error: %s", BlitzenCore::CE_MESH_SYSTEM_NAME, UPLOAD_MESH_TO_DISK_RES_TO_STRING(uploadRes));
            return false;
        }
        context.ResetMeshContext();

        return true;
    }

    LOAD_MESH_FROM_DISK_RES LoadMeshFromDisk(const char* meshName, BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE& memoryMappedFile, MeshResources& context)
    {
        if (context.m_meshPrimitives.m_LODCount != 0 || context.m_triangles.m_vertexCount != 0 || context.m_triangles.m_vtxIdxCount != 0)
        {
            return LOAD_MESH_FROM_DISK_RES::LOD_CONTEXT_NOT_RESET_BEFORE_LOAD_OPERATION;
        }

        BlitCL::String stringContainer;
        const char* meshPath = GetRpfMeshPath(meshName, stringContainer);

        constexpr size_t LCStartOfRPFOffset = 0;

        auto mmfRes{ memoryMappedFile.OpenRead(meshPath) };
        if (BlitzenPlatform::CHECK_BLIT_MMF_RES_FOR_ERROR(mmfRes))
        {
            BLIT_FATAL("%s: Failed to open Rapid Resource File for mesh read. Received Platform Error: %s", BlitzenCore::CE_MESH_SYSTEM_NAME, BlitzenPlatform::GET_BLIT_MMF_RES_ERROR_STR(mmfRes));
            return LOAD_MESH_FROM_DISK_RES::FAILED_TO_OPEN_FILE;
        }

        BLIT_RPF_MESH_FILE_HEADER_ARR headerArr{};
        if (!BlitzenPlatform::ReadMemoryMappedFile(memoryMappedFile, LCStartOfRPFOffset, GCRapidMeshFileHeaderElementCount * sizeof(size_t), headerArr))
        {
            return LOAD_MESH_FROM_DISK_RES::FAILED_TO_READ_HEADER;
        }

        uint32_t surfaceID = context.m_meshPrimitives.AddSurfaceToMap();
        auto pMeshPrimitive = &context.m_meshPrimitives.m_meshPrimitives[surfaceID];
        if (!BlitzenPlatform::ReadMemoryMappedFile(memoryMappedFile, headerArr[BLIT_RPF_MESH_FILE_HEADER_MESH_PRIMITIVE_ID], sizeof(PrimitiveSurface), pMeshPrimitive))
        {
            return LOAD_MESH_FROM_DISK_RES::FAILED_TO_READ_MESH_PRIMITIVE;
        }

        auto pMeshPrimitiveData = &context.m_meshPrimitives.m_meshPrimitiveData[surfaceID];
        if (!BlitzenPlatform::ReadMemoryMappedFile(memoryMappedFile, headerArr[BLIT_RPF_MESH_FILE_HEADER_MESH_PRIMITIVE_DATA_ID], sizeof(MeshPrimitiveData), pMeshPrimitiveData))
        {
            return LOAD_MESH_FROM_DISK_RES::FAILED_TO_READ_MESH_PRIMITIVE_NON_SHADER_DATA;
        }

        context.m_meshPrimitives.m_LODCount = pMeshPrimitive->lodCount;
        LodData* pLODArr = context.m_meshPrimitives.m_LODs;
        if (!BlitzenPlatform::ReadMemoryMappedFile(memoryMappedFile, headerArr[BLIT_RPF_MESH_FILE_HEADER_LOD_DATA_ID], sizeof(LodData) * pMeshPrimitive->lodCount, pLODArr))
        {
            return LOAD_MESH_FROM_DISK_RES::FAILED_TO_READ_LOD_DATA;
        }

        auto pBoundingSphereData = &context.m_meshPrimitives.m_boundingSpheres[surfaceID];
        if (!BlitzenPlatform::ReadMemoryMappedFile(memoryMappedFile, headerArr[BLIT_RPF_MESH_FILE_HEADER_BOUNDING_SPHERE_ID], sizeof(BoundingSphere), pBoundingSphereData))
        {
            return LOAD_MESH_FROM_DISK_RES::FAILED_TO_READ_VISIBILITY_BOUNDING_SPHERE;
        }

        if (!BlitzenPlatform::ReadMemoryMappedFile(memoryMappedFile, headerArr[BLIT_RPF_MESH_FILE_HEADER_VTX_POS_DATA_ID], sizeof(VtxPos) * pMeshPrimitiveData->m_primitiveVertexCount,
            context.m_triangles.m_vertexPositions))
        {
            return LOAD_MESH_FROM_DISK_RES::FAILED_TO_READ_VERTEX_POSITIONS;
        }
        
        if (!BlitzenPlatform::ReadMemoryMappedFile(memoryMappedFile, headerArr[BLIT_RPF_MESH_FILE_HEADER_VTX_NRM_DATA_ID], sizeof(VtxNormals) * pMeshPrimitiveData->m_primitiveVertexCount,
            context.m_triangles.m_vertexNormals))
        {
            return LOAD_MESH_FROM_DISK_RES::FAILED_TO_READ_VERTEX_NORMALS;
        }

        if (!BlitzenPlatform::ReadMemoryMappedFile(memoryMappedFile, headerArr[BLIT_RPF_MESH_FILE_HEADER_VTX_TNG_DATA_ID], sizeof(VtxTangents) * pMeshPrimitiveData->m_primitiveVertexCount,
            context.m_triangles.m_vertexTangents))
        {
            return LOAD_MESH_FROM_DISK_RES::FAILED_TO_READ_VERTEX_TANGENTS;
        }

        if (!BlitzenPlatform::ReadMemoryMappedFile(memoryMappedFile, headerArr[BLIT_RPF_MESH_FILE_HEADER_VTX_COORD_DATA_ID], sizeof(VtxTexCoords) * pMeshPrimitiveData->m_primitiveVertexCount,
            context.m_triangles.m_vertexUVs))
        {
            return LOAD_MESH_FROM_DISK_RES::FAILED_TO_READ_VERTEX_TEXTURE_COORDINATES;
        }

        // Saves vertex count for upload
        context.m_triangles.m_vertexCount = pMeshPrimitiveData->m_primitiveVertexCount;

        uint32_t indexCount = 0;
        for (uint32_t lod = 0; lod < pMeshPrimitive->lodCount; ++lod)
        {
            indexCount += pLODArr[lod].indexCount;
        }
        if (!BlitzenPlatform::ReadMemoryMappedFile(memoryMappedFile, headerArr[BLIT_RPF_MESH_FILE_HEADER_VTX_IDXS_DATA_ID], sizeof(uint32_t) * indexCount, context.m_triangles.m_indices))
        {
            return LOAD_MESH_FROM_DISK_RES::FAILED_TO_READ_VERTEX_INDICES;
        }

        context.m_triangles.m_vtxIdxCount = indexCount;

        // Checks if clusters were loaded
        if (headerArr[BLIT_RPF_MESH_FILE_HEADER_CLUSTER_VTXS_DATA_ID] != 0)
        {
            uint32_t clusterCount = 0;
            for (uint32_t lod = 0; lod < pMeshPrimitive->lodCount; ++lod)
            {
                clusterCount += pLODArr[lod].clusterCount;
            }

            if (!BlitzenPlatform::ReadMemoryMappedFile(memoryMappedFile, headerArr[BLIT_RPF_MESH_FILE_HEADER_VTX_IDXS_DATA_ID], sizeof(ClusterVertices) * clusterCount, 
                context.m_clusters.m_clusterVertices))
            {
                return LOAD_MESH_FROM_DISK_RES::FAILED_TO_READ_CLUSTER_VERTICES;
            }

            if (!BlitzenPlatform::ReadMemoryMappedFile(memoryMappedFile, headerArr[BLIT_RPF_MESH_FILE_HEADER_CLUSTER_SPHERES_DATA_ID], sizeof(ClusterSphere) * clusterCount, 
                context.m_clusters.m_clusterSpheres))
            {
                return LOAD_MESH_FROM_DISK_RES::FAILED_TO_READ_CLUSTER_SPHERES;
            }

            if (!BlitzenPlatform::ReadMemoryMappedFile(memoryMappedFile, headerArr[BLIT_RPF_MESH_FILE_HEADER_CLUSTER_CONES_DATA_ID], sizeof(ClusterCone) * clusterCount,
                context.m_clusters.m_clusterCones))
            {
                return LOAD_MESH_FROM_DISK_RES::FAILED_TO_READ_CLUSTER_CONES;
            }

            if (!BlitzenPlatform::ReadMemoryMappedFile(memoryMappedFile, headerArr[BLIT_RPF_MESH_FILE_HEADER_CLUSTER_CONES_DATA_ID], sizeof(uint32_t) * indexCount,
                context.m_clusters.m_clusterIndices))
            {
                return LOAD_MESH_FROM_DISK_RES::FAILED_TO_READ_CLUSTER_INDICES;
            }

            context.m_clusters.m_clusterCount = clusterCount;
        }

        // Holds collider data as well, this data will not be passed to the GPU but it might be used to create the colliders for objects that have not created them themselves
        if (!BlitzenPlatform::ReadMemoryMappedFile(memoryMappedFile, headerArr[BLIT_RPF_MESH_FILE_HEADER_COLLIDER_AMAXRAD_DATA_ID], sizeof(ColliderAMaxRad),
            &context.m_meshPrimitives.mColliders[surfaceID].AMaxRad))
        {
            return LOAD_MESH_FROM_DISK_RES::FAILED_TO_READ_COLLIDER_AMAXRAD_DATA;
        }

        if (!BlitzenPlatform::ReadMemoryMappedFile(memoryMappedFile, headerArr[BLIT_RPF_MESH_FILE_HEADER_COLLIDER_BMINTYPE_DATA_ID], sizeof(ColliderBMinType),
            &context.m_meshPrimitives.mColliders[surfaceID].BMinType))
        {
            return LOAD_MESH_FROM_DISK_RES::FAILED_TO_READ_COLLIDER_BMINTYPE_DATA;
        }
            
        return LOAD_MESH_FROM_DISK_RES::SUCCESS;
    }

    UPLOAD_MESH_TO_DISK_RES UploadMeshToDisk(const char* meshName, BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE& memoryMappedFile, MeshResources& context, bool clustersBuildFlag)
    {
        BlitCL::String stringContainer;
        const char* meshPath = GetRpfMeshPath(meshName, stringContainer);

        constexpr size_t LCStartOfRPFOffset = 0;
        constexpr size_t LCMeshPrimitiveCountPerRPF = 1;

        // Gets the required file size for the current resource
        size_t writeSize{ GetRpfMeshSize(context, clustersBuildFlag) };
        if (writeSize == CE_GET_RPF_MESH_SIZE_ERROR_RETURN_CODE)
        {
            return UPLOAD_MESH_TO_DISK_RES::UPLOAD_SIZE_TOO_BIG;
        }

        auto mmfRes{ memoryMappedFile.OpenWrite(meshPath, (DWORD)writeSize) };
        if (BlitzenPlatform::CHECK_BLIT_MMF_RES_FOR_ERROR(mmfRes))
        {
            BLIT_FATAL("%s: Failed to open Rapid Resource File. Received Platform Error: %s", BlitzenCore::CE_MESH_SYSTEM_NAME, BlitzenPlatform::GET_BLIT_MMF_RES_ERROR_STR(mmfRes));
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_OPEN_FILE;
        }

        // After getting the size the memory mapped file is opened for write operations
        // Starts below the header. The header will be written last
        // All resources needed to use the mesh primitive are written and their offset is saved to the header struct
        size_t fileOffset = 0;
        fileOffset += GcRapidMeshFileHeaderSize;
        BLIT_RPF_MESH_FILE_HEADER_ARR headerArr{ 0 };
        // Opens memory mapped file for writing
        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, LCMeshPrimitiveCountPerRPF * sizeof(PrimitiveSurface), &context.m_meshPrimitives.m_meshPrimitives[0]))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_MESH_PRIMITIVES;
        }
        headerArr[BLIT_RPF_MESH_FILE_HEADER_MESH_PRIMITIVE_ID] = fileOffset;
        fileOffset += LCMeshPrimitiveCountPerRPF * sizeof(PrimitiveSurface);

        // Mesh primitives (head of data)
        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, LCMeshPrimitiveCountPerRPF * sizeof(MeshPrimitiveData), &context.m_meshPrimitives.m_meshPrimitiveData[0]))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_MESH_PRIMITIVE_DATA;
        }
        headerArr[BLIT_RPF_MESH_FILE_HEADER_MESH_PRIMITIVE_DATA_ID] = fileOffset;
        fileOffset += LCMeshPrimitiveCountPerRPF * sizeof(MeshPrimitiveData);

        // Mesh primitive has one or more levels of detail
		size_t lodDataSize = context.m_meshPrimitives.m_LODCount * sizeof(LodData);
        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, lodDataSize, &context.m_meshPrimitives.m_LODs))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_LOD_DATA;
        }
        headerArr[BLIT_RPF_MESH_FILE_HEADER_LOD_DATA_ID] = fileOffset;
        fileOffset += lodDataSize;

        // Mesh primitive can generate bounding sphere based on its vertices
		size_t boundingSphereDataSize = LCMeshPrimitiveCountPerRPF * sizeof(BoundingSphere);
        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, boundingSphereDataSize, &context.m_meshPrimitives.m_boundingSpheres[0]))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_BOUNDING_SPHERE_DATA;
        }
        headerArr[BLIT_RPF_MESH_FILE_HEADER_BOUNDING_SPHERE_ID] = fileOffset;
        fileOffset += boundingSphereDataSize;

		size_t vertexPositionsDataSize = context.m_triangles.m_vertexCount * sizeof(VtxPos);
        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, vertexPositionsDataSize, context.m_triangles.m_vertexPositions))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_VERTEX_POSITIONS_DATA;
        }
        headerArr[BLIT_RPF_MESH_FILE_HEADER_VTX_POS_DATA_ID] = fileOffset;
        fileOffset += vertexPositionsDataSize;

		size_t vertexNormalsDataSize = context.m_triangles.m_vertexCount * sizeof(VtxNormals);
        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, vertexNormalsDataSize, context.m_triangles.m_vertexNormals))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_VERTEX_NORMALS_DATA;
        }
        headerArr[BLIT_RPF_MESH_FILE_HEADER_VTX_NRM_DATA_ID] = fileOffset;
        fileOffset += vertexNormalsDataSize;

		size_t vertexTangentsDataSize = context.m_triangles.m_vertexCount * sizeof(VtxTangents);
        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, vertexTangentsDataSize, context.m_triangles.m_vertexTangents))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_VERTEX_TANGENTS_DATA;
        }
        headerArr[BLIT_RPF_MESH_FILE_HEADER_VTX_TNG_DATA_ID] = fileOffset;
        fileOffset += vertexTangentsDataSize;

		size_t vertexTexCoordsDataSize = context.m_triangles.m_vertexCount * sizeof(VtxTexCoords);
        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, vertexTexCoordsDataSize,context.m_triangles.m_vertexUVs))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_VERTEX_TEXTURE_COORDINATES_DATA;
        }
        headerArr[BLIT_RPF_MESH_FILE_HEADER_VTX_COORD_DATA_ID] = fileOffset;
        fileOffset += vertexTexCoordsDataSize;

		size_t vertexIndicesDataSize = context.m_triangles.m_vtxIdxCount * sizeof(uint32_t);
        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, vertexIndicesDataSize, context.m_triangles.m_indices))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_VERTEX_INDICES;
        }
        headerArr[BLIT_RPF_MESH_FILE_HEADER_VTX_IDXS_DATA_ID] = fileOffset;
        fileOffset += vertexIndicesDataSize;

        if (clustersBuildFlag)
        {
			size_t clusterVerticesDataSize = context.m_clusters.m_clusterCount * sizeof(ClusterVertices);
            if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, clusterVerticesDataSize, context.m_clusters.m_clusterVertices))
            {
                return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_CLUSTER_VERTEX_DATA;
            }
            headerArr[BLIT_RPF_MESH_FILE_HEADER_CLUSTER_VTXS_DATA_ID] = fileOffset;
            fileOffset += clusterVerticesDataSize;

			size_t clusterSpheresDataSize = context.m_clusters.m_clusterCount * sizeof(ClusterSphere);
            if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, clusterSpheresDataSize, context.m_clusters.m_clusterSpheres))
            {
                return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_CLUSTER_SPHERES;
            }
            headerArr[BLIT_RPF_MESH_FILE_HEADER_CLUSTER_SPHERES_DATA_ID] = fileOffset;
            fileOffset += clusterSpheresDataSize;

			size_t clusterConesDataSize = context.m_clusters.m_clusterCount * sizeof(ClusterCone);
            if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, clusterConesDataSize, context.m_clusters.m_clusterCones))
            {
                return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_CLUSTER_CONES;
            }
            headerArr[BLIT_RPF_MESH_FILE_HEADER_CLUSTER_CONES_DATA_ID] = fileOffset;
            fileOffset += clusterConesDataSize;

			size_t clusterIdxDataSize = context.m_triangles.m_vtxIdxCount * sizeof(uint32_t);
            if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, clusterIdxDataSize, context.m_clusters.m_clusterIndices))
            {
                return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_CLUSTER_INDICES;
            }
            headerArr[BLIT_RPF_MESH_FILE_HEADER_CLUSTER_IDXS_DATA_ID] = fileOffset;
            fileOffset += clusterIdxDataSize;
        }
        else
        {
            headerArr[BLIT_RPF_MESH_FILE_HEADER_CLUSTER_VTXS_DATA_ID] = 0;
            headerArr[BLIT_RPF_MESH_FILE_HEADER_CLUSTER_SPHERES_DATA_ID] = 0;
            headerArr[BLIT_RPF_MESH_FILE_HEADER_CLUSTER_CONES_DATA_ID] = 0;
            headerArr[BLIT_RPF_MESH_FILE_HEADER_CLUSTER_IDXS_DATA_ID] = 0;
        }

		size_t colliderAMaxRadDataSize = LCMeshPrimitiveCountPerRPF * sizeof(ColliderAMaxRad);
        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, colliderAMaxRadDataSize, &context.m_meshPrimitives.mColliders[0].AMaxRad))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_COLLIDER_AMAXRAD_DATA;
        }
        headerArr[BLIT_RPF_MESH_FILE_HEADER_COLLIDER_AMAXRAD_DATA_ID] = fileOffset;
        fileOffset += colliderAMaxRadDataSize;

        size_t colliderBMinTypeDataSize = LCMeshPrimitiveCountPerRPF * sizeof(ColliderBMinType);
        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, colliderBMinTypeDataSize, &context.m_meshPrimitives.mColliders[0].BMinType))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_COLLIDER_BMINTYPE_DATA;
        }
        headerArr[BLIT_RPF_MESH_FILE_HEADER_COLLIDER_BMINTYPE_DATA_ID] = fileOffset;
        fileOffset += colliderBMinTypeDataSize;

        // Write to the head at the end
        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, LCStartOfRPFOffset, GCRapidMeshFileHeaderElementCount * sizeof(size_t), headerArr))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_OFFSETS_TO_HEADER;
        }

        return UPLOAD_MESH_TO_DISK_RES::SUCCESS;
    }

    bool LoadImportedSceneNodesFromDisk(const char* sceneName, uint32_t& outResourceCount, uint32_t& outNodesCount, uint32_t resourceCountLimit, uint32_t nodesCountLimit,
        BlitzenCore::BLIT_PTR& outRenderObjects, BlitzenCore::BLIT_PTR& outMeshTransforms)
    {
        BlitCL::String stringContainer;
        const char* filePath = BuildImportedSceneNodesFilepath(sceneName, stringContainer);

        BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE memoryMappedFile;
        auto openReadRes = memoryMappedFile.OpenRead(filePath);
        if (BlitzenPlatform::CHECK_BLIT_MMF_RES_FOR_ERROR(openReadRes))
        {
            BLIT_ERROR("%s: Failed to open memory mapped file for imported scene nodes read", BlitzenCore::CE_SCENE_SYSTEM_NAME);
            return false;
        }

        ImportedSceneNodesHeaderArr headerArr;
        const uint32_t StartOfFile = 0;
        if (!BlitzenPlatform::ReadMemoryMappedFile(memoryMappedFile, StartOfFile, GCImportedSceneNodesHeaderElementCount * sizeof(size_t), headerArr))
        {
            BLIT_ERROR("%s: Failed to read header for imported scene nodes file", BlitzenCore::CE_SCENE_SYSTEM_NAME);
            return false;
        }

        outResourceCount = (uint32_t)headerArr[BlitRpfImportedSceneNodesHeaderResourceCountID];
        if (outResourceCount > resourceCountLimit)
        {
			BLIT_ERROR("%s: Additional Resource count from scene \"%s\" exceeds the limit", BlitzenCore::CE_SCENE_SYSTEM_NAME, sceneName);
            return false;
        }

        outNodesCount = (uint32_t)headerArr[BlitRpfImportedSceneNodesHeaderNodeCountID];
        if (outNodesCount > nodesCountLimit)
        {
			BLIT_ERROR("%s: Additional Nodes count from scene \"%s\" exceeds the limit", BlitzenCore::CE_SCENE_SYSTEM_NAME, sceneName);
            return false;
        }

        size_t renderObjectsOffset = headerArr[BlitRpfImportedSceneNodesHeaderRenderObjectsID];
        size_t transformsOffset = headerArr[BlitRpfImportedSceneNodesHeaderTransformsID];
        size_t renderObjectsSize = outNodesCount * sizeof(RenderObject);
        size_t transformsSize = outNodesCount * sizeof(MeshTransform);
        
        outRenderObjects.Init(renderObjectsSize);
		outMeshTransforms.Init(transformsSize);
        auto renderObjectsPtr = reinterpret_cast<RenderObject*>(outRenderObjects.mPtr);
		auto meshTransformsPtr = reinterpret_cast<MeshTransform*>(outMeshTransforms.mPtr);

        if (!BlitzenPlatform::ReadMemoryMappedFile(memoryMappedFile, renderObjectsOffset, renderObjectsSize, renderObjectsPtr))
        {
            BLIT_ERROR("%s: Failed to read render objects from imported scene nodes file", BlitzenCore::CE_SCENE_SYSTEM_NAME);
            return false;
        }

        if (!BlitzenPlatform::ReadMemoryMappedFile(memoryMappedFile, transformsOffset, transformsSize, meshTransformsPtr))
        {
            BLIT_ERROR("%s: Failed to read mesh transforms from imported scene nodes file", BlitzenCore::CE_SCENE_SYSTEM_NAME);
            return false;
        }

        return true;
    }

    bool UploadImportedSceneNodesToDisk(const char* sceneName, uint32_t resourceCount, uint32_t nodesCount, RenderObject* renderObjects, MeshTransform* meshTransforms)
    {
        BlitCL::String stringContainer;
        const char* filePath = BuildImportedSceneNodesFilepath(sceneName, stringContainer);

        size_t headerWriteSize = GCImportedSceneNodesHeaderElementCount * sizeof(size_t);
        size_t nodesWriteSize = nodesCount * sizeof(RenderObject);
        size_t transformsWriteSize = nodesCount * sizeof(MeshTransform);

        size_t predictedSize = headerWriteSize + nodesWriteSize + transformsWriteSize;

        BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE memoryMappedFile;
        auto openWriteRes = memoryMappedFile.OpenWrite(filePath, (uint32_t)predictedSize);
        if (BlitzenPlatform::CHECK_BLIT_MMF_RES_FOR_ERROR(openWriteRes))
        {
            BLIT_ERROR("%s: Failed to open memory mapped file for imported scene nodes write", BlitzenCore::CE_SCENE_SYSTEM_NAME);
            return false;
        }

        ImportedSceneNodesHeaderArr headerArr;

        headerArr[BlitRpfImportedSceneNodesHeaderResourceCountID] = resourceCount;
        headerArr[BlitRpfImportedSceneNodesHeaderNodeCountID] = nodesCount;

        size_t fileOffset = headerWriteSize;
        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, nodesWriteSize, renderObjects))
        {
            BLIT_ERROR("%s: Failed to upload render objects imported scene nodes file", BlitzenCore::CE_SCENE_SYSTEM_NAME);
            return false;
        }
        headerArr[BlitRpfImportedSceneNodesHeaderRenderObjectsID] = fileOffset;
        fileOffset += nodesWriteSize;

        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, transformsWriteSize, meshTransforms))
        {
            BLIT_ERROR("%s: Failed to upload world transforms imported scene nodes file", BlitzenCore::CE_SCENE_SYSTEM_NAME);
            return false;
        }
        headerArr[BlitRpfImportedSceneNodesHeaderTransformsID] = fileOffset;
        fileOffset += transformsWriteSize;

        const uint32_t StartOfFile = 0;
        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, StartOfFile, headerWriteSize, headerArr))
        {
            BLIT_ERROR("%s: Failed to upload header for imported scene nodes file");
            return false;
        }

        return true;
    }
}