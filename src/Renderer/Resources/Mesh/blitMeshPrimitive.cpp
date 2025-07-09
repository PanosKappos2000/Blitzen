#include "blitMeshes.h"
// Algorithms for building meshlets, loading LODs, optimizing vertex caches etc.
// https://github.com/zeux/meshoptimizer
#include "Meshoptimizer/meshoptimizer.h"
#include "BlitCL/blitDynamicArr.h"
#include "BlitzenMathLibrary/blitML.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenEngine
{
    bool PrimitiveContainer::AddVertices(Vertex* vertices, uint32_t count)
    {
        if (m_vertexCount + count > BlitzenCore::Ce_MaxWorldVertexCount)
        {
            BLIT_ERROR("%s: Exceeded max world vertex count", BlitzenCore::CE_MESH_SYSTEM_NAME);
            return false;
        }

        BlitzenCore::BlitMemCopy(&m_vertices[m_vertexCount], vertices, count * sizeof(Vertex));
        m_vertexCount += count;

        return true;
    }

    bool PrimitiveContainer::AddIndices(uint32_t* indices, uint32_t count)
    {
        if (m_vtxIdxCount + count > BlitzenCore::Ce_MaxWorldVertexIndicesCount)
        {
            BLIT_ERROR("%s: Surpassed max world vertex indices count", BlitzenCore::CE_MESH_SYSTEM_NAME);
            return false;
        }

        BlitzenCore::BlitMemCopy(&m_indices[m_vtxIdxCount], indices, sizeof(uint32_t) * count);
        m_vtxIdxCount += count;

        return true;
    }

    SurfaceCreateRes MeshPrimitivesContainer::GenerateSurface(PrimitiveContainer& primitives, ClusterContainer& clusters, MESH_PRIMITIVE_CREATE_CONTEXT& context)
	{
        if (m_meshPrimitivesCount >= BlitzenCore::Ce_MaxMeshPrimitivesCount)
        {
            BLIT_ERROR("%s: Exceeded max allowed mesh primitive count", BlitzenCore::CE_MESH_SYSTEM_NAME);
            return SurfaceCreateRes::MAX_SURFACE_COUNT_REACHED;
        }

        // Optimize vertices and indices using meshoptimizer
        meshopt_optimizeVertexCache(context.m_indices, context.m_indices, context.m_indexCount, context.m_vertexCount);
        meshopt_optimizeVertexFetch(context.m_vertices, context.m_indices, context.m_indexCount, context.m_vertices, context.m_vertexCount, sizeof(Vertex));
        
        // Adds vertex offset and vertex count
        m_meshPrimitiveData[m_meshPrimitivesCount].m_primitiveVertexOffset = primitives.m_vertexCount;
        m_meshPrimitiveData[m_meshPrimitivesCount].m_primitiveVertexCount = context.m_vertexCount;

        // Vertices read to be added after optimize
        if (!primitives.AddVertices(context.m_vertices, context.m_vertexCount))
        {
            return SurfaceCreateRes::SURFACE_VERTICES_COULD_NOT_BE_ADDED;
        }

        auto& newSurface{ m_meshPrimitives[m_meshPrimitivesCount] };

        MESH_PRIMITIVE_LOD_CREATE_CONTEXT lodContext{};
        lodContext.m_pClusters = &clusters;
        lodContext.m_pMeshPrimitiveInfo = &context;
        lodContext.m_pPrimitives = &primitives;
        lodContext.m_vertexOffset = m_meshPrimitiveData[m_meshPrimitivesCount].m_primitiveVertexOffset;
        if (!GenerateLODs(newSurface, lodContext))
        {
            return SurfaceCreateRes::LOD_GENERATION_FAILED;
        }

        // Generates bounding sphere for surface, will be taken later by any render object using this surface
        // This might be potentially wasteful when it come to memory, but accessing the vertices of a surface, outside of this funtion, to generate the sphere, would be a bit of a pain
        GenerateBoundingSphere(newSurface, m_boundingSpheres[m_meshPrimitivesCount], context);

        newSurface.materialId = context.m_materialID;

        if (context.m_specialFlags & MESH_PRIMITIVE_SPECIAL_TRANSPARENT)
        {
            m_meshPrimitiveData[m_meshPrimitivesCount].m_primitiveTransparencyFlags = BlitzenCore::BB_TRUE;
        }
        else
        {
            m_meshPrimitiveData[m_meshPrimitivesCount].m_primitiveTransparencyFlags = BlitzenCore::BB_FALSE;
        }

        m_meshPrimitivesCount++;

        return SurfaceCreateRes::SUCCESS;
	}

    bool MeshPrimitivesContainer::GenerateLODs(PrimitiveSurface& surface, MESH_PRIMITIVE_LOD_CREATE_CONTEXT& context)
    {
        // Automatic LOD generation helpers
        BlitCL::DynamicArray<BlitML::vec3> normals{ context.m_pMeshPrimitiveInfo->m_vertexCount };
        for (size_t i = 0; i < normals.GetSize(); ++i)
        {
            auto& v = context.m_pMeshPrimitiveInfo->m_vertices[i];
            normals[i] = BlitML::vec3(v.normalX / 127.f - 1.f, v.normalY / 127.f - 1.f, v.normalZ / 127.f - 1.f);
        }

        //float lodScale = meshopt_simplifyScale(&context.m_pMeshPrimitiveInfo->m_vertices[0].position.x, context.m_pMeshPrimitiveInfo->m_vertexCount, sizeof(Vertex));
        float lodScale = BlitGenerator::GetLODDegradationScale(context.m_pMeshPrimitiveInfo->m_vertices, context.m_pMeshPrimitiveInfo->m_vertexCount);

        float lodError = 0.f;

        float normalWeights[3] = { 1.f, 1.f, 1.f };

        // Adds the starter indices to a dynamic LOD array
        BlitCL::DynamicArray<uint32_t> lodIndices{ context.m_pMeshPrimitiveInfo->m_indexCount };
        BlitzenCore::BlitMemCopy(lodIndices.Data(), context.m_pMeshPrimitiveInfo->m_indices, sizeof(uint32_t) * context.m_pMeshPrimitiveInfo->m_indexCount);

        // FINAL
        BlitCL::DynamicArray<uint32_t> allLodIndices;

        // First LOD for this surface
        surface.lodOffset = m_LODCount;

        while (surface.lodCount < BlitzenCore::Ce_MaxLodCountPerSurface)
        {
            if (m_LODCount >= CE_MAX_LOD_COUNT)
            {
                BLIT_ERROR("%s: Max world LOD count has been reached / exceeded", BlitzenCore::CE_MESH_SYSTEM_NAME);
                return false;
            }

            surface.lodCount++;

            auto& lod{m_LODs[m_LODCount++]};
            lod.firstIndex = context.m_pPrimitives->m_vtxIdxCount + (uint32_t)allLodIndices.GetSize();
            lod.indexCount = uint32_t(lodIndices.GetSize());

            // CLUSTER OFFSET
            lod.clusterOffset = context.m_pClusters->m_clusterCount;

            // NEW CLUSTER
            if (BlitzenCore::Ce_BuildClusters)
            {
                LOD_CLUSTERS_CREATE_CONTEXT clustersCtx{};
                clustersCtx.m_vertices = context.m_pMeshPrimitiveInfo->m_vertices;
                clustersCtx.m_vertexCount = context.m_pMeshPrimitiveInfo->m_vertexCount;
                clustersCtx.m_indices = lodIndices.Data();
                clustersCtx.m_indicesCount = uint32_t(lodIndices.GetSize());
                uint32_t clusterCount = GenerateClusters(clustersCtx, context.m_vertexOffset, context.m_pClusters);
                if (clusterCount == CE_MAX_WORLD_CLUSTER_COUNT)
                {
                    BLIT_ERROR("%s: Cluster generation returned error cluster count", BlitzenCore::CE_MESH_SYSTEM_NAME);
                    return false;
                }
                lod.clusterCount = clusterCount;
            }

            lod.error = lodError * lodScale;

            // Adds current lod indices
            allLodIndices.AppendArray(lodIndices);

            // Starts generating the next level of detail
            if (surface.lodCount < BlitzenCore::Ce_MaxLodCountPerSurface)
            {
                size_t nextIndicesTarget = static_cast<size_t>((double(lodIndices.GetSize()) * 0.65) / 3) * 3;
                const float maxError = 1e-1f;
                float nextError = 0.f;

                //BlitGenerator::LOD_DEGRADE_CONTEXT lodDegradationContext{};
                //lodDegradationContext.m_previousIndices = lodIndices.Data();
                //lodDegradationContext.m_indexCount = (uint32_t)lodIndices.GetSize();
                //lodDegradationContext.m_vertexArr = context.m_pMeshPrimitiveInfo->m_vertices;
                //lodDegradationContext.m_vertexCount = context.m_pMeshPrimitiveInfo->m_vertexCount;
                //lodDegradationContext.m_vtxNormalsArr = normals.Data();
                //lodDegradationContext.m_attributeWeightsArr = normalWeights;
                //lodDegradationContext.m_attribute32BITCount = 3;
                //uint32_t blitNext = BlitGenerator::DegradeLevelOfDetail(lodIndices.Data(), (uint32_t)lodIndices.GetSize(), lodDegradationContext, maxError, &nextError);
                //if (blitNext == BlitGenerator::CE_DEGRADATION_ERROR_CODE)
                //{
                //    BLIT_WARN("%s: LOD degradation returned error code", BlitzenCore::CE_MESH_SYSTEM_NAME);
                //    break;
                //}
                //if (blitNext > lodIndices.GetSize())
                //{
                //    BLIT_WARN("%s: Next LOD index count that was generated after degradation, was bigger than the previous lod size", BlitzenCore::CE_MESH_SYSTEM_NAME);
                //    break;
                //}

                // Gets the size of the next level of detail
                size_t nextIndicesSize = meshopt_simplifyWithAttributes(lodIndices.Data(), lodIndices.Data(), lodIndices.GetSize(), &context.m_pMeshPrimitiveInfo->m_vertices[0].position.x,
                    context.m_pMeshPrimitiveInfo->m_vertexCount, sizeof(Vertex), &normals[0].x, sizeof(BlitML::vec3), normalWeights, 3, nullptr, nextIndicesTarget, maxError, 0, &nextError);

                if (nextIndicesSize > lodIndices.GetSize())
                {
                    BLIT_WARN("%s: Next LOD index count that was generated after degradation, was bigger than the previous lod size", BlitzenCore::CE_MESH_SYSTEM_NAME);
                    break;
                }
                // Reached the error bounds
                if (nextIndicesSize == lodIndices.GetSize() || nextIndicesSize == 0)
                {
                    BLIT_WARN("%s: Next LOD has hit error bounds", BlitzenCore::CE_MESH_SYSTEM_NAME);
                    break;
                }
                
                if (nextIndicesSize >= size_t(double(lodIndices.GetSize()) * 0.95))
                {
                    BLIT_WARN("%s: LOD skipped ", BlitzenCore::CE_MESH_SYSTEM_NAME)
                    break;
                }

                // Resize and optimize
                lodIndices.Resize(nextIndicesSize);
                meshopt_optimizeVertexCache(lodIndices.Data(), lodIndices.Data(), lodIndices.GetSize(), context.m_pMeshPrimitiveInfo->m_vertexCount);

                // since it starts from next lod accumulate the error
                lodError = BlitML::Max(lodError, nextError);
            }
        }

        if (surface.lodCount > BlitzenCore::Ce_MaxLodCountPerSurface)
        {
            BLIT_ERROR("A surface has loaded too many LODs");
            return false;
        }

        for (uint32_t& idx : allLodIndices)
        {
            idx += context.m_vertexOffset;
        }

        if (!context.m_pPrimitives->AddIndices(allLodIndices.Data(), uint32_t(allLodIndices.GetSize())))
        {
            BLIT_ERROR("%s: Failed to load all LOD idices", BlitzenCore::CE_MESH_SYSTEM_NAME);
            return false;
        }

        return true;
    }

    uint32_t MeshPrimitivesContainer::GenerateClusters(LOD_CLUSTERS_CREATE_CONTEXT& context, uint32_t vertexOffset, ClusterContainer* pClusters)
    {
        BlitCL::DynamicArray<meshopt_Meshlet> meshop_meshlets{ meshopt_buildMeshletsBound(context.m_indicesCount, CE_MAX_VERTICES_PER_CLUSTER, CE_MAX_TRIANGLES_PER_CLUSTER) };

        BlitCL::DynamicArray<uint32_t> meshletVertices{ meshop_meshlets.GetSize() * CE_MAX_VERTICES_PER_CLUSTER };

        BlitCL::DynamicArray<unsigned char> meshletTriangles{ meshop_meshlets.GetSize() * CE_MAX_TRIANGLES_PER_CLUSTER * 3 };

        meshop_meshlets.Resize(meshopt_buildMeshlets(meshop_meshlets.Data(), meshletVertices.Data(), meshletTriangles.Data(), context.m_indices, context.m_indicesCount,
            &context.m_vertices[0].position.x, context.m_vertexCount, sizeof(Vertex), CE_MAX_VERTICES_PER_CLUSTER, CE_MAX_TRIANGLES_PER_CLUSTER,
            CE_CLUSTER_CONE_WEIGHT));

        if (pClusters->m_clusterCount + uint32_t(meshop_meshlets.GetSize()) > CE_MAX_WORLD_CLUSTER_COUNT)
        {
            BLIT_ERROR("%s: Max cluster count exceeded", BlitzenCore::CE_MESH_SYSTEM_NAME);
            return CE_MAX_WORLD_CLUSTER_COUNT;
        }

        for (size_t i = 0; i < meshop_meshlets.GetSize(); ++i)
        {
            auto& meshlet = meshop_meshlets[i];

            meshopt_optimizeMeshlet(&meshletVertices[meshlet.vertex_offset], &meshletTriangles[meshlet.triangle_offset], meshlet.triangle_count, meshlet.vertex_count);

#if defined(BLIT_MESH_SHADERS)

            size_t dataOffset = m_clusterIndices.GetSize();
            for (uint32_t i = 0; i < meshlet.vertex_count; ++i)
            {
                m_clusterIndices.PushBack(meshletVertices[meshlet.vertex_offset + i]);
            }
            uint32_t indexGroups = reinterpret_cast<uint32_t*>(&meshletTriangles[0] + meshlet.triangle_offset);
            uint32_t indexGroupCount = meshlet.triangle_count * 3;
            for (uint32_t i = 0; i < indexGroupCount; ++i)
            {
                m_clusterIndices.PushBack(indexGroups[size_t(i)]);
            }

#else

            uint32_t dataOffset = pClusters->m_clusterIndicesCount;

            if (pClusters->m_clusterIndicesCount + meshlet.triangle_count * 3 > BlitzenCore::Ce_MaxWorldVertexIndicesCount)
            {
                BLIT_ERROR("%s: Surpassed max vertex indices count while generating cluster", BlitzenCore::CE_MESH_SYSTEM_NAME);
                return CE_MAX_WORLD_CLUSTER_COUNT;
            }

            const uint32_t* vertexLookup = &meshletVertices[meshlet.vertex_offset];
            const unsigned char* triangles = &meshletTriangles[meshlet.triangle_offset];
            for (uint32_t t = 0; t < meshlet.triangle_count; ++t)
            {
                // Each triangle has 3 indices into the local meshlet vertex array
                for (uint32_t j = 0; j < 3; ++j)
                {
                    uint32_t localIndex = triangles[t * 3 + j];
                    uint32_t globalIndex = vertexLookup[localIndex] + vertexOffset;

                    pClusters->m_clusterIndices[pClusters->m_clusterIndicesCount++] = globalIndex;
                }
            }

#endif

            auto bounds = meshopt_computeMeshletBounds(&meshletVertices[meshlet.vertex_offset], &meshletTriangles[meshlet.triangle_offset], meshlet.triangle_count,
                &context.m_vertices[0].position.x, context.m_vertexCount, sizeof(Vertex));

            auto& cluster{ pClusters->m_clusters[pClusters->m_clusterCount++] };
            cluster.dataOffset = uint32_t(dataOffset);
            cluster.triangleCount = meshlet.triangle_count;
            cluster.vertexCount = meshlet.vertex_count;

            cluster.center = BlitML::vec3(bounds.center[0], bounds.center[1], bounds.center[2]);
            cluster.radius = bounds.radius;
            cluster.coneAxisX = bounds.cone_axis_s8[0];
            cluster.coneAxisY = bounds.cone_axis_s8[1];
            cluster.coneAxisZ = bounds.cone_axis_s8[2];
            cluster.coneCutoff = bounds.cone_cutoff_s8;
        }

        return (uint32_t)meshop_meshlets.GetSize();
    }

    void MeshPrimitivesContainer::GenerateBoundingSphere(PrimitiveSurface& surface, BoundingSphere& surfaceBounds, MESH_PRIMITIVE_CREATE_CONTEXT& context)
    {
        BlitML::vec3 center{ 0.f };
        for (size_t i = 0; i < context.m_vertexCount; ++i)
        {
            center = center + context.m_vertices[i].position;
        }
        center = center / float(context.m_vertexCount);

        // Bounding sphere radius
        float radius = 0;
        for (size_t i = 0; i < context.m_vertexCount; ++i)
        {
            const auto& pos = context.m_vertices[i].position;
            radius = BlitML::Max(radius, BlitML::Distance(center, BlitML::vec3(pos.x, pos.y, pos.z)));
        }
        surfaceBounds.m_center = center;
        surfaceBounds.m_radius = radius;
    }

    void MeshPrimitivesContainer::GenerateTangents(MESH_PRIMITIVE_CREATE_CONTEXT& context)
    {
        for (size_t i = 0; i < context.m_indexCount; i += 3)
        {
            auto i0 = context.m_indices[i + 0];
            auto i1 = context.m_indices[i + 1];
            auto i2 = context.m_indices[i + 2];

            auto edge1 = context.m_vertices[i1].position - context.m_vertices[i0].position;
            auto edge2 = context.m_vertices[i2].position - context.m_vertices[i0].position;

            auto deltaU1 = float(context.m_vertices[i1].uvX - context.m_vertices[i0].uvX);
            auto deltaV1 = float(context.m_vertices[i1].uvY - context.m_vertices[i0].uvY);

            auto deltaU2 = float(context.m_vertices[i2].uvX - context.m_vertices[i0].uvX);
            auto deltaV2 = float(context.m_vertices[i2].uvY - context.m_vertices[i0].uvY);

            float dividend = (deltaU1 * deltaV2 - deltaU2 * deltaV1);

            float fc = 1.0f / dividend;

            BlitML::vec3 tangent
            {
                (fc * (deltaV2 * edge1.x - deltaV1 * edge2.x)),
                (fc * (deltaV2 * edge1.y - deltaV1 * edge2.y)),
                (fc * (deltaV2 * edge1.z - deltaV1 * edge2.z))
            };

            BlitML::Normalize(tangent);

            float sx = deltaU1, sy = deltaU2;

            float tx = deltaV1, ty = deltaV2;

            float handedness = ((tx * sy - ty * sx) < 0.0f) ? -1.0f : 1.0f;

            BlitML::vec4 t4{ tangent, handedness };

            context.m_vertices[i0].tangentX = uint8_t(t4.x * 127.f + 127.5f);
            context.m_vertices[i0].tangentY = uint8_t(t4.y * 127.f + 127.5f);
            context.m_vertices[i0].tangentZ = uint8_t(t4.z * 127.f + 127.5f);
            context.m_vertices[i0].tangentW = uint8_t(t4.w * 127.f + 127.5f);

            context.m_vertices[i1].tangentX = uint8_t(t4.x * 127.f + 127.5f);
            context.m_vertices[i1].tangentY = uint8_t(t4.y * 127.f + 127.5f);
            context.m_vertices[i1].tangentZ = uint8_t(t4.z * 127.f + 127.5f);
            context.m_vertices[i1].tangentW = uint8_t(t4.w * 127.f + 127.5f);

            context.m_vertices[i2].tangentX = uint8_t(t4.x * 127.f + 127.5f);
            context.m_vertices[i2].tangentY = uint8_t(t4.y * 127.f + 127.5f);
            context.m_vertices[i2].tangentZ = uint8_t(t4.z * 127.f + 127.5f);
            context.m_vertices[i2].tangentW = uint8_t(t4.w * 127.f + 127.5f);
        }
    }

    SurfaceCreateRes MeshPrimitivesContainer::GenerateMeshPrimitive(PrimitiveContainer& primitives, ClusterContainer clusters, MESH_PRIMITIVE_GENERATE_CONTEXT& context)
    {
        if (m_meshPrimitivesCount >= BlitzenCore::Ce_MaxMeshPrimitivesCount)
        {
            BLIT_ERROR("%s: Exceeded max allowed mesh primitive count", BlitzenCore::CE_MESH_SYSTEM_NAME);
            return SurfaceCreateRes::MAX_SURFACE_COUNT_REACHED;
        }
        
        // Adds vertex offset and vertex count
        m_meshPrimitiveData[m_meshPrimitivesCount].m_primitiveVertexOffset = primitives.m_vertexCount;
        m_meshPrimitiveData[m_meshPrimitivesCount].m_primitiveVertexCount = context.m_vertexCount;

        auto& newSurface{ m_meshPrimitives[m_meshPrimitivesCount] };

        MESH_PRIMITIVE_LOD_GENERATE_CONTEXT lodContext{};
        lodContext.m_pClusters = &clusters;
        lodContext.m_pMeshPrimitiveInfo = &context;
        lodContext.m_pPrimitives = &primitives;
        lodContext.m_vertexOffset = m_meshPrimitiveData[m_meshPrimitivesCount].m_primitiveVertexOffset;
        if (!GenerateMeshPrimitiveLODIndices(newSurface, lodContext))
        {
            return SurfaceCreateRes::LOD_GENERATION_FAILED;
        }

        // Generates bounding sphere for surface, will be taken later by any render object using this surface
        // This might be potentially wasteful when it come to memory, but accessing the vertices of a surface, outside of this funtion, to generate the sphere, would be a bit of a pain
        GenerateBoundingSphere(m_boundingSpheres[m_meshPrimitivesCount], context.m_pVertexContext->m_vtxPosArr, context.m_vertexCount);

        newSurface.materialId = context.m_materialID;

        if (context.m_specialFlags & MESH_PRIMITIVE_SPECIAL_TRANSPARENT)
        {
            m_meshPrimitiveData[m_meshPrimitivesCount].m_primitiveTransparencyFlags = BlitzenCore::BB_TRUE;
        }
        else
        {
            m_meshPrimitiveData[m_meshPrimitivesCount].m_primitiveTransparencyFlags = BlitzenCore::BB_FALSE;
        }

        m_meshPrimitivesCount++;

        return SurfaceCreateRes::SUCCESS;
    }

    bool MeshPrimitivesContainer::GenerateMeshPrimitiveLODIndices(PrimitiveSurface& surface, MESH_PRIMITIVE_LOD_GENERATE_CONTEXT& context)
    {
        float lodScale = meshopt_simplifyScale(&context.m_pMeshPrimitiveInfo->m_pVertexContext->m_vtxPosArr[0].x, context.m_pMeshPrimitiveInfo->m_vertexCount, sizeof(Vertex));
        //float lodScale = BlitGenerator::GetLODDegradationScale(context.m_pMeshPrimitiveInfo->m_vertices, context.m_pMeshPrimitiveInfo->m_vertexCount);

        float lodError = 0.f;

        float normalWeights[3] = { 1.f, 1.f, 1.f };

        // Adds the starter indices to a dynamic LOD array
        BlitCL::DynamicArray<uint32_t> lodIndices{ context.m_pMeshPrimitiveInfo->m_indexCount };
        BlitzenCore::BlitMemCopy(lodIndices.Data(), context.m_pMeshPrimitiveInfo->m_indices, sizeof(uint32_t) * context.m_pMeshPrimitiveInfo->m_indexCount);

        // FINAL
        BlitCL::DynamicArray<uint32_t> allLodIndices;

        while (surface.lodCount < BlitzenCore::Ce_MaxLodCountPerSurface)
        {
            if (m_LODCount >= CE_MAX_LOD_COUNT)
            {
                BLIT_ERROR("%s: Max world LOD count has been reached / exceeded", BlitzenCore::CE_MESH_SYSTEM_NAME);
                return false;
            }

            surface.lodCount++;

            auto& lod{ m_LODs[m_LODCount++] };
            lod.firstIndex = (uint32_t)allLodIndices.GetSize();
            lod.indexCount = uint32_t(lodIndices.GetSize());

            // CLUSTER OFFSET
            lod.clusterOffset = context.m_pClusters->m_clusterCount;

            // NEW CLUSTER
            //if (BlitzenCore::Ce_BuildClusters)
            //{
            //    LOD_CLUSTERS_CREATE_CONTEXT clustersCtx{};
            //    clustersCtx.m_vertices = context.m_pMeshPrimitiveInfo->m_indices;
            //    clustersCtx.m_vertexCount = context.m_pMeshPrimitiveInfo->m_vertexCount;
            //    clustersCtx.m_indices = lodIndices.Data();
            //    clustersCtx.m_indicesCount = uint32_t(lodIndices.GetSize());
            //    uint32_t clusterCount = GenerateClusters(clustersCtx, context.m_vertexOffset, context.m_pClusters);
            //    if (clusterCount == CE_MAX_WORLD_CLUSTER_COUNT)
            //    {
            //        BLIT_ERROR("%s: Cluster generation returned error cluster count", BlitzenCore::CE_MESH_SYSTEM_NAME);
            //        return false;
            //    }
            //    lod.clusterCount = clusterCount;
            //}

            lod.error = lodError * lodScale;

            // Adds current lod indices
            allLodIndices.AppendArray(lodIndices);

            // Starts generating the next level of detail
            if (surface.lodCount < BlitzenCore::Ce_MaxLodCountPerSurface)
            {
                size_t nextIndicesTarget = static_cast<size_t>((double(lodIndices.GetSize()) * 0.65) / 3) * 3;
                const float maxError = 1e-1f;
                float nextError = 0.f;

                //BlitGenerator::LOD_DEGRADE_CONTEXT lodDegradationContext{};
                //lodDegradationContext.m_previousIndices = lodIndices.Data();
                //lodDegradationContext.m_indexCount = (uint32_t)lodIndices.GetSize();
                //lodDegradationContext.m_vertexArr = context.m_pMeshPrimitiveInfo->m_vertices;
                //lodDegradationContext.m_vertexCount = context.m_pMeshPrimitiveInfo->m_vertexCount;
                //lodDegradationContext.m_vtxNormalsArr = normals.Data();
                //lodDegradationContext.m_attributeWeightsArr = normalWeights;
                //lodDegradationContext.m_attribute32BITCount = 3;
                //uint32_t blitNext = BlitGenerator::DegradeLevelOfDetail(lodIndices.Data(), (uint32_t)lodIndices.GetSize(), lodDegradationContext, maxError, &nextError);
                //if (blitNext == BlitGenerator::CE_DEGRADATION_ERROR_CODE)
                //{
                //    BLIT_WARN("%s: LOD degradation returned error code", BlitzenCore::CE_MESH_SYSTEM_NAME);
                //    break;
                //}
                //if (blitNext > lodIndices.GetSize())
                //{
                //    BLIT_WARN("%s: Next LOD index count that was generated after degradation, was bigger than the previous lod size", BlitzenCore::CE_MESH_SYSTEM_NAME);
                //    break;
                //}

                // Gets the size of the next level of detail
                size_t nextIndicesSize = meshopt_simplifyWithAttributes(lodIndices.Data(), lodIndices.Data(), lodIndices.GetSize(), &context.m_pMeshPrimitiveInfo->m_pVertexContext->m_vtxPosArr[0].x,
                    context.m_pMeshPrimitiveInfo->m_vertexCount, sizeof(Vertex), &context.m_pMeshPrimitiveInfo->m_pVertexContext->m_vtxNrmArr[0].x, 
                    sizeof(BlitML::vec3), normalWeights, 3, nullptr, nextIndicesTarget, maxError, 0, &nextError);

                if (nextIndicesSize > lodIndices.GetSize())
                {
                    BLIT_WARN("%s: Next LOD index count that was generated after degradation, was bigger than the previous lod size", BlitzenCore::CE_MESH_SYSTEM_NAME);
                    break;
                }
                // Reached the error bounds
                if (nextIndicesSize == lodIndices.GetSize() || nextIndicesSize == 0)
                {
                    BLIT_WARN("%s: Next LOD has hit error bounds", BlitzenCore::CE_MESH_SYSTEM_NAME);
                    break;
                }

                if (nextIndicesSize >= size_t(double(lodIndices.GetSize()) * 0.95))
                {
                    BLIT_WARN("%s: LOD skipped ", BlitzenCore::CE_MESH_SYSTEM_NAME)
                        break;
                }

                // Resize and optimize
                lodIndices.Resize(nextIndicesSize);
                meshopt_optimizeVertexCache(lodIndices.Data(), lodIndices.Data(), lodIndices.GetSize(), context.m_pMeshPrimitiveInfo->m_vertexCount);

                // since it starts from next lod accumulate the error
                lodError = BlitML::Max(lodError, nextError);
            }
        }

        if (surface.lodCount > BlitzenCore::Ce_MaxLodCountPerSurface)
        {
            BLIT_ERROR("A surface has loaded too many LODs");
            return false;
        }

        if (!context.m_pPrimitives->AddIndices(allLodIndices.Data(), uint32_t(allLodIndices.GetSize())))
        {
            BLIT_ERROR("%s: Failed to load all LOD idices", BlitzenCore::CE_MESH_SYSTEM_NAME);
            return false;
        }

        return true;
    }

    void MeshPrimitivesContainer::GenerateBoundingSphere(BoundingSphere& surfaceBounds, BlitML::vec3* vtxPosArr, uint32_t vtxCount)
    {
        BlitML::vec3 center{ 0.f };
        for (size_t i = 0; i < vtxCount; ++i)
        {
            center = center + vtxPosArr[i];
        }
        center = center / float(vtxCount);

        // Bounding sphere radius
        float radius = 0;
        for (size_t i = 0; i < vtxCount; ++i)
        {
            const auto& pos = vtxPosArr[i];
            radius = BlitML::Max(radius, BlitML::Distance(center, BlitML::vec3(pos.x, pos.y, pos.z)));
        }
        surfaceBounds.m_center = center;
        surfaceBounds.m_radius = radius;
    }
}