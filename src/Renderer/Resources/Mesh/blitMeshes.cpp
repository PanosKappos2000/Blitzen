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

namespace BlitzenEngine
{
    inline MeshResources* P_MESH_RESOURCES = nullptr;

    void InitializeMeshResourcesPointer_STATIC_ACCESS(MeshResources* ptr)
    {
        BLIT_ASSERT_MESSAGE(P_MESH_RESOURCES == nullptr, "Attempted to reinitialize mesh resources singleton pointer");

        P_MESH_RESOURCES = ptr;
    }

    BoundingSphere* GetBoundingSphereResources_STATIC_ACCESS(Mesh* pMesh)
    {
        return &P_MESH_RESOURCES->m_meshPrimitives.m_boundingSpheres[pMesh->firstSurface];
    }

    Mesh& RequestMeshResources_STATIC_ACCESS(const char* meshName)
    {
		return P_MESH_RESOURCES->m_meshMap[meshName];
    }

    BlitzenCore::FAT_BOOL GetMeshPrimitiveTransparencyFlag_STATIC_ACCESS(uint32_t surfaceID)
    {
        return P_MESH_RESOURCES->m_meshPrimitives.m_meshPrimitiveData[surfaceID].m_primitiveTransparencyFlags;
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

        if (file.f_size * 3 > BlitzenCore::Ce_MaxWorldVertexIndicesCount)
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

    uint32_t LoadObjFileMeshToDisk(MeshResources& context, const char* filename, const char* meshName)
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

        if (file.f_size * 3 > BlitzenCore::Ce_MaxWorldVertexIndicesCount)
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

        meshopt_optimizeVertexCache(indices.Data(), indices.Data(), indexCount, vertexCount);
        meshopt_optimizeVertexFetch(vertices.Data(), indices.Data(), indexCount, vertices.Data(), vertexCount, sizeof(Vertex));

        HLSL_VTX_CONTEXT hlslVerticesContext{};
        hlslVerticesContext.m_vtxPosArr = context.m_triangles.m_vertexPositions;
        hlslVerticesContext.m_vtxNrmArr = context.m_triangles.m_vertexNormals;
        hlslVerticesContext.m_vtxTngArr = context.m_triangles.m_vertexTangents;
        hlslVerticesContext.m_texCoordArr = context.m_triangles.m_vertexUVs;
        ConvertClassicVerticesToHlslFormat(hlslVerticesContext, vertices.Data(), vertexCount);

        MESH_PRIMITIVE_GENERATE_CONTEXT primitiveCreateCtx{};
        primitiveCreateCtx.m_indexCount = (uint32_t)indices.GetSize();
        primitiveCreateCtx.m_indices = indices.Data();
        primitiveCreateCtx.m_vertexCount = (uint32_t)vertices.GetSize();
        primitiveCreateCtx.m_pVertexContext = &hlslVerticesContext;
        auto meshPrimitiveGenRes{ context.m_meshPrimitives.GenerateMeshPrimitive(context.m_triangles, context.m_clusters, primitiveCreateCtx) };
        if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)meshPrimitiveGenRes))
        {
            BlitzenCore::LOG_ERROR_MSG_AND_RETURN(BlitzenCore::CE_MESH_SYSTEM_NAME, MESH_PRIMITIVE_CREATE_RES_TO_STRING(meshPrimitiveGenRes));
            return BlitzenCore::Ce_MaxMeshCount;
        }
    }

    /****************
    *    DRAFT!     *
    *****************/
    uint32_t LoadMeshFromDisk()
    {
        struct RESOURCE_LOADER_CONTEXT
        {
            uint32_t meshCount;
            uint32_t vertexCount;
            uint32_t surfaceCount;
            uint32_t lodCount;
            uint32_t vtxIdxCount;
            uint32_t clusterCount;
        };

        void* pPool = nullptr;// Write current data here (this will be allocated from another system)
        uint32_t currentPoolIdx = 0;

        void* pShaderDataPool = nullptr;// Final true data.

        uint32_t meshDataOffset = 0, meshDataCount = 0;
        uint32_t mPrimitiveDataOffset = 0, mPrimitiveDataCount = 0;
        uint32_t lodDataOffset = 0, lodDataCount = 0;

        // Get mesh accessor from file
        uint32_t offset = 0; // Form file
        uint32_t count = 1; // From file
        // Loop surfaces
        
        struct SurfaceAccessor// Prototype
        {
            uint32_t vertexOffset;
            uint32_t vertexCount;
            uint32_t lodOffset;
            uint32_t lodCount;
            BlitzenCore::FAT_BOOL transparencyFlag; // Or maybe have a seperate file for transparent resources
            uint32_t materialID; // Maybe this does not belong here anymore
        };
        // memcpy(reinterpret_cast<SurfaceAccessor>(pool)[currentPoolIdx], reinterpret_cast<SurfaceAccessor>(file)[offset], sizeof(SurfaceAccessor) * surfaceCount;
        currentPoolIdx += count;

        struct LodAccessor// Prototype
        {
            uint32_t indexOffset;
            uint32_t indexCount;
            uint32_t clusterOffset;
            uint32_t clusterCount;
            float lodError;
        };
        // memcpy(reinterpret_cast<LodAccessor(pool)[currentPoolIdx], reinterpret_cast<LodAccessor

        // memcpy(pPool + meshDataOffet * sizeof(Mesh), mappedFile + meshDataOffset, 

        Mesh* meshDataArr = &reinterpret_cast<Mesh*>(pPool)[meshDataOffset];

        return 0;

        // Clean Data Context.Clean() maybe?
        // Zero out everything, do not deallocate pool, it will be reused 
    }

    UPLOAD_MESH_TO_DISK_RES UploadMeshToDisk(const char* meshName, BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE memoryMappedFile, UPLOAD_MESH_RPF_CONTEXT& rpfCtx)
    {
        BlitCL::String meshPath{ meshName };
        char* extension = const_cast<char*>(CE_BLITZEN_RAPID_MESH_FILE_EXTENSION);
        meshPath.Append(extension);

        size_t writeSize(GetRpfMeshSize(rpfCtx));
        if (writeSize == CE_GET_RPF_MESH_SIZE_ERROR_RETURN_CODE)
        {
            return UPLOAD_MESH_TO_DISK_RES::UPLOAD_SIZE_TOO_BIG;
        }

        auto mmfRes{ memoryMappedFile.OpenWrite(meshPath.GetClassic(), writeSize) };
        if (BlitzenPlatform::CHECK_BLIT_MMF_RES_FOR_ERROR(mmfRes))
        {
            BLIT_FATAL("%s: Failed to open Rapid Resource File. Received Platform Error: %s", BlitzenCore::CE_MESH_SYSTEM_NAME, BlitzenPlatform::GET_BLIT_MMF_RES_ERROR_STR(mmfRes));
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_OPEN_FILE;
        }

        size_t fileOffset = 0;
        fileOffset += CE_BLITZEN_RAPID_MESH_FILE_HEADER_SIZE;
        BLIT_RPF_MESH_FILE_HEADER_ARR headerArr{ 0 };

        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, rpfCtx.m_meshPrimitiveCount * sizeof(PrimitiveSurface), rpfCtx.m_surfaceArray))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_MESH_PRIMITIVES;
        }
        fileOffset += rpfCtx.m_meshPrimitiveCount * sizeof(PrimitiveSurface);
        headerArr[BLIT_RPF_MESH_FILE_HEADER_MESH_PRIMITIVE_ID] = fileOffset;

        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, rpfCtx.m_meshPrimitiveCount * sizeof(MeshPrimitiveData), rpfCtx.m_meshPrimitiveData))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_MESH_PRIMITIVE_DATA;
        }
        fileOffset += rpfCtx.m_meshPrimitiveCount * sizeof(MeshPrimitiveData);
        headerArr[BLIT_RPF_MESH_FILE_HEADER_MESH_PRIMITIVE_DATA_ID] = fileOffset;

        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, rpfCtx.m_lodCount * sizeof(LodData), rpfCtx.m_lodDataArr))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_LOD_DATA;
        }
        fileOffset += rpfCtx.m_lodCount * sizeof(LodData);
        headerArr[BLIT_RPF_MESH_FILE_HEADER_LOD_DATA_ID] = fileOffset;

        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, rpfCtx.m_meshPrimitiveCount * sizeof(BoundingSphere), rpfCtx.m_boundsArr))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_BOUNDING_SPHERE_DATA;
        }
        fileOffset += rpfCtx.m_meshPrimitiveCount * sizeof(BoundingSphere);
        headerArr[BLIT_RPF_MESH_FILE_HEADER_BOUNDING_SPHERE_ID] = fileOffset;

        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, rpfCtx.m_vtxCount * sizeof(VtxPos), rpfCtx.m_vtxPosArr))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_VERTEX_POSITIONS_DATA;
        }
        fileOffset += rpfCtx.m_vtxCount * sizeof(VtxPos);
        headerArr[BLIT_RPF_MESH_FILE_HEADER_VTX_POS_DATA_ID] = fileOffset;

        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, rpfCtx.m_vtxCount * sizeof(VtxNormals), rpfCtx.m_vtxNormalsArr))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_VERTEX_NORMALS_DATA;
        }
        fileOffset += rpfCtx.m_vtxCount * sizeof(VtxNormals);
        headerArr[BLIT_RPF_MESH_FILE_HEADER_VTX_NRM_DATA_ID] = fileOffset;

        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, rpfCtx.m_vtxCount * sizeof(VtxTangents), rpfCtx.m_vtxTangArr))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_VERTEX_TANGENTS_DATA;
        }
        fileOffset += rpfCtx.m_vtxCount * sizeof(VtxTangents);
        headerArr[BLIT_RPF_MESH_FILE_HEADER_VTX_TNG_DATA_ID] = fileOffset;

        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, rpfCtx.m_vtxCount * sizeof(VtxTexCoords), rpfCtx.m_vtxTexCoordArr))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_VERTEX_TEXTURE_COORDINATES_DATA;
        }
        fileOffset += rpfCtx.m_vtxCount * sizeof(VtxTexCoords);
        headerArr[BLIT_RPF_MESH_FILE_HEADER_VTX_COORD_DATA_ID] = fileOffset;

        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, rpfCtx.m_idxCount * sizeof(uint32_t), rpfCtx.m_vtxIdxArr))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_VERTEX_INDICES;
        }
        fileOffset += rpfCtx.m_idxCount * sizeof(uint32_t);
        headerArr[BLIT_RPF_MESH_FILE_HEADER_VTX_IDXS_DATA_ID] = fileOffset;

        if (rpfCtx.m_clustersBuiltFlag)
        {
            if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, rpfCtx.m_clusterCount * sizeof(ClusterVertices), rpfCtx.m_clusterVtxArr))
            {
                return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_CLUSTER_VERTEX_DATA;
            }
            fileOffset += rpfCtx.m_clusterCount * sizeof(ClusterVertices);
            headerArr[BLIT_RPF_MESH_FILE_HEADER_CLUSTER_VTXS_DATA_ID] = fileOffset;

            if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, rpfCtx.m_clusterCount * sizeof(ClusterSphere), rpfCtx.m_clusterSphereArr))
            {
                return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_CLUSTER_SPHERES;
            }
            fileOffset += rpfCtx.m_clusterCount * sizeof(ClusterSphere);
            headerArr[BLIT_RPF_MESH_FILE_HEADER_CLUSTER_SPHERES_DATA_ID] = fileOffset;

            if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, rpfCtx.m_clusterCount * sizeof(ClusterCone), rpfCtx.m_clusterConeArr))
            {
                return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_CLUSTER_CONES;
            }
            fileOffset += rpfCtx.m_clusterCount * sizeof(ClusterCone);
            headerArr[BLIT_RPF_MESH_FILE_HEADER_CLUSTER_CONES_DATA_ID] = fileOffset;

            if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, fileOffset, rpfCtx.m_idxCount * sizeof(uint32_t), rpfCtx.m_clusterIdxArr))
            {
                return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_CLUSTER_INDICES;
            }
            fileOffset += rpfCtx.m_idxCount * sizeof(uint32_t);
            headerArr[BLIT_RPF_MESH_FILE_HEADER_CLUSTER_IDXS_DATA_ID] = fileOffset;
        }

        if (!BlitzenPlatform::WriteMemoryMappedFile(memoryMappedFile, 0, CE_BLIT_RPF_MESH_FILE_HEADER_ELEMENT_COUNT * sizeof(size_t), headerArr))
        {
            return UPLOAD_MESH_TO_DISK_RES::FAILED_TO_UPLOAD_OFFSETS_TO_HEADER;
        }

        return UPLOAD_MESH_TO_DISK_RES::SUCCESS;
    }
}