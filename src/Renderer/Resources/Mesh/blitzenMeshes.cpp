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

namespace BlitzenEngine
{
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
}