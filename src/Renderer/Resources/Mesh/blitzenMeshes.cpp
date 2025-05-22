#include "blitMeshes.h"
// Algorithms for building meshlets, loading LODs, optimizing vertex caches etc.
// https://github.com/zeux/meshoptimizer
#include "Meshoptimizer/meshoptimizer.h"
// Used for loading .obj meshes
// https://github.com/thisistherk/fast_obj
#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"
#include "objparser.h"

namespace BlitzenEngine
{
    bool LoadMeshFromObj(MeshResources& context, const char* filename, const char* meshName)
    {
        // The function should return if the engine will go over the max allowed mesh assets
        if (context.m_meshCount > BlitzenCore::Ce_MaxMeshCount)
        {
            BLIT_ERROR("Max mesh count: ( %i ) reached!", BlitzenCore::Ce_MaxMeshCount);
            return 0;
        }

        BLIT_INFO("Loading obj model form file: %s", filename);

        // Get the current mesh and give it the size surface array as its first surface index
        auto& currentMesh = context.m_meshes[context.m_meshCount];
        currentMesh.firstSurface = static_cast<uint32_t>(context.m_surfaces.GetSize());
        currentMesh.meshId = uint32_t(context.m_meshCount);
        context.m_meshMap.Insert(meshName, currentMesh);

        ObjFile file;
        if (!objParseFile(file, filename))
        {
            return 0;
        }

        size_t indexCount = file.f_size / 3;

        BlitCL::DynamicArray<Vertex> triangleVertices(indexCount);

        BLIT_INFO("Loading vertices and indices");

        for (size_t i = 0; i < indexCount; ++i)
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
            vtx.normalX = static_cast<uint8_t>(normalX * 127.f + 127.5f);
            vtx.normalY = static_cast<uint8_t>(normalY * 127.f + 127.5f);
            vtx.normalZ = static_cast<uint8_t>(normalZ * 127.f + 127.5f);

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

        GenerateTangents(vertices, indices);

        BLIT_INFO("Creating surface");
        GenerateSurface(context, vertices, indices);

        currentMesh.surfaceCount++;// Increment the surface count
        ++context.m_meshCount;// Increment the mesh count

        return 1;
    }

    void GenerateSurface(MeshResources& context, BlitCL::DynamicArray<Vertex>& surfaceVertices, BlitCL::DynamicArray<uint32_t>& surfaceIndices)
    {
        // Optimize vertices and indices using meshoptimizer
        meshopt_optimizeVertexCache(surfaceIndices.Data(), surfaceIndices.Data(), surfaceIndices.GetSize(), surfaceVertices.GetSize());
        meshopt_optimizeVertexFetch(surfaceVertices.Data(), surfaceIndices.Data(), surfaceIndices.GetSize(), surfaceVertices.Data(),
            surfaceVertices.GetSize(), sizeof(Vertex));

        PrimitiveSurface newSurface{};
        newSurface.vertexOffset = uint32_t(context.m_vertices.GetSize());
        context.m_vertices.AppendArray(surfaceVertices);

        BLIT_INFO("Generating Level Of Detail Indices");
        GenerateLODs(context, newSurface, surfaceVertices, surfaceIndices);

        BLIT_INFO("Generating bounding sphere");
        GenerateBoundingSphere(newSurface, surfaceVertices, surfaceIndices);

        // TODO: Add logic for material without relying on gltf
        newSurface.materialId = 0;

        // Adds the surface to the global surfaces array
        context.m_surfaces.PushBack(newSurface);
        context.m_primitiveVertexCounts.PushBack(uint32_t(context.m_vertices.GetSize()));

        // Default, if a caller wants transparency, they should handle it
        context.m_bTransparencyList.PushBack({ false });
    }

    void GenerateLODs(MeshResources& context, PrimitiveSurface& surface, BlitCL::DynamicArray<Vertex>& surfaceVertices, BlitCL::DynamicArray<uint32_t>& surfaceIndices)
    {
        // Automatic LOD generation helpers
        BlitCL::DynamicArray<BlitML::vec3> normals{ surfaceVertices.GetSize() };
        for (size_t i = 0; i < surfaceVertices.GetSize(); ++i)
        {
            auto& v = surfaceVertices[i];
            normals[i] = BlitML::vec3(v.normalX / 127.f - 1.f, v.normalY / 127.f - 1.f, v.normalZ / 127.f - 1.f);
        }
        float lodScale = meshopt_simplifyScale(&surfaceVertices[0].position.x, surfaceVertices.GetSize(), sizeof(Vertex));
        float lodError = 0.f;
        float normalWeights[3] = { 1.f, 1.f, 1.f };

        // Pass the original loaded indices of the surface to the new lod indices
        BlitCL::DynamicArray<uint32_t> lodIndices{ surfaceIndices };
        BlitCL::DynamicArray<uint32_t> allLodIndices;

        surface.lodOffset = static_cast<uint32_t>(context.m_LODs.GetSize());
        while (surface.lodCount < BlitzenCore::Ce_MaxLodCountPerSurface)
        {
            surface.lodCount++;
            BLIT_INFO("Generating LOD %d", surface.lodCount);

            // Instancing
            LodInstanceCounter lodInstanceCounter{};
            lodInstanceCounter.instanceOffset = uint32_t(context.m_LODs.GetSize() * BlitzenCore::Ce_MaxInstanceCountPerLOD);
            context.m_lodInstanceList.PushBack(lodInstanceCounter);

            LodData lod{};
            lod.firstIndex = static_cast<uint32_t>(context.m_indices.GetSize() + allLodIndices.GetSize());
            lod.indexCount = static_cast<uint32_t>(lodIndices.GetSize());

            // TODO: Might want to make LODs include one or the other, indices and clusters are not used together
            lod.clusterOffset = static_cast<uint32_t>(context.m_clusters.GetSize());
            lod.clusterCount = BlitzenCore::Ce_BuildClusters ? static_cast<uint32_t>(GenerateClusters(context, surfaceVertices, lodIndices, surface.vertexOffset)) : 0;

            lod.error = lodError * lodScale;
            context.m_LODs.PushBack(lod);

            // Adds current lod indices
            allLodIndices.AppendArray(lodIndices);

            // Starts generating the next level of detail
            if (surface.lodCount < BlitzenCore::Ce_MaxLodCountPerSurface)
            {
                auto nextIndicesTarget = static_cast<size_t>((double(lodIndices.GetSize()) * 0.65) / 3) * 3;
                const float maxError = 1e-1f;
                float nextError = 0.f;

                // Gets the size of the next level of detail
                auto nextIndicesSize = meshopt_simplifyWithAttributes(lodIndices.Data(), lodIndices.Data(), lodIndices.GetSize(), &surfaceVertices[0].position.x,
                    surfaceVertices.GetSize(), sizeof(Vertex), &normals[0].x, sizeof(BlitML::vec3), normalWeights, 3, nullptr, nextIndicesTarget, maxError,
                    0, &nextError);

                if (nextIndicesSize > lodIndices.GetSize())
                {
                    BLIT_ERROR("LOD generation failed");
                    break;
                }
                // Reached the error bounds
                if (nextIndicesSize == lodIndices.GetSize() || nextIndicesSize == 0)
                {
                    break;
                }
                // while I could keep this LOD, it's too close to the last one 
                // (and it can't go below that due to constant error bound above)
                if (nextIndicesSize >= size_t(double(lodIndices.GetSize()) * 0.95))
                {
                    break;
                }

                // Resize and optimize
                lodIndices.Resize(nextIndicesSize);
                meshopt_optimizeVertexCache(lodIndices.Data(), lodIndices.Data(), lodIndices.GetSize(), surfaceVertices.GetSize());

                // since it starts from next lod accumulate the error
                lodError = BlitML::Max(lodError, nextError);
            }
        }

        if (surface.lodCount > BlitzenCore::Ce_MaxLodCountPerSurface)
        {
            BLIT_ERROR("A surface has loaded too many LODs");
        }

        // Adds vertex offset before appending all lods to the global indices array
        auto vertexOffset = surface.vertexOffset;
        for (auto& index : allLodIndices)
        {
            index += vertexOffset;
        }
        context.m_indices.AppendArray(allLodIndices);
    }

    // Loads cluster using the meshoptimizer library
    size_t GenerateClusters(MeshResources& context, BlitCL::DynamicArray<Vertex>& inVertices, BlitCL::DynamicArray<uint32_t>& inIndices, uint32_t vertexOffset)
    {
        BLIT_INFO("Generating clusters");

        const size_t maxVertices = 64;
        const size_t maxTriangles = 124;
        const float coneWeight = 0.25f;

        BlitCL::DynamicArray<meshopt_Meshlet> akMeshlets{ meshopt_buildMeshletsBound(inIndices.GetSize(), maxVertices, maxTriangles) };
        BlitCL::DynamicArray<unsigned int> meshletVertices{ akMeshlets.GetSize() * maxVertices };
        BlitCL::DynamicArray<unsigned char> meshletTriangles{ akMeshlets.GetSize() * maxTriangles * 3 };

        akMeshlets.Resize(meshopt_buildMeshlets(akMeshlets.Data(), meshletVertices.Data(), meshletTriangles.Data(), inIndices.Data(), inIndices.GetSize(),
            &inVertices[0].position.x, inVertices.GetSize(), sizeof(Vertex), maxVertices, maxTriangles, coneWeight));


        for (size_t i = 0; i < akMeshlets.GetSize(); ++i)
        {
            auto& meshlet = akMeshlets[i];

            meshopt_optimizeMeshlet(&meshletVertices[meshlet.vertex_offset],
                &meshletTriangles[meshlet.triangle_offset],
                meshlet.triangle_count, meshlet.vertex_count);

            /*auto dataOffset = m_clusterIndices.GetSize();
            for(unsigned int i = 0; i < meshlet.vertex_count; ++i)
            {
                m_clusterIndices.PushBack(meshletVertices[meshlet.vertex_offset + i]);
            }
            auto indexGroups = reinterpret_cast<unsigned int*>(
                &meshletTriangles[0] + meshlet.triangle_offset);
            unsigned int indexGroupCount = meshlet.triangle_count * 3;
            for(unsigned int i = 0; i < indexGroupCount; ++i)
            {
                m_clusterIndices.PushBack(indexGroups[size_t(i)]);
            }*/

            const unsigned int* vertexLookup = &meshletVertices[meshlet.vertex_offset];
            const unsigned char* triangles = &meshletTriangles[meshlet.triangle_offset];
            auto dataOffset = context.m_clusterIndices.GetSize();
            for (unsigned int t = 0; t < meshlet.triangle_count; ++t)
            {
                // Each triangle has 3 indices into the local meshlet vertex array
                for (int j = 0; j < 3; ++j)
                {
                    unsigned int localIndex = triangles[t * 3 + j];
                    unsigned int globalIndex = vertexLookup[localIndex] + vertexOffset;
                    context.m_clusterIndices.PushBack(globalIndex);
                }
            }

            auto bounds = meshopt_computeMeshletBounds(&meshletVertices[meshlet.vertex_offset],
                &meshletTriangles[meshlet.triangle_offset], meshlet.triangle_count, &inVertices[0].position.x, inVertices.GetSize(), sizeof(Vertex));

            Cluster cluster{};
            cluster.dataOffset = static_cast<uint32_t>(dataOffset);
            cluster.triangleCount = meshlet.triangle_count;
            cluster.vertexCount = meshlet.vertex_count;

            cluster.center = BlitML::vec3(bounds.center[0], bounds.center[1], bounds.center[2]);
            cluster.radius = bounds.radius;
            cluster.coneAxisX = bounds.cone_axis_s8[0];
            cluster.coneAxisY = bounds.cone_axis_s8[1];
            cluster.coneAxisZ = bounds.cone_axis_s8[2];
            cluster.coneCutoff = bounds.cone_cutoff_s8;

            context.m_clusters.PushBack(cluster);
        }

        return akMeshlets.GetSize();
    }

    void GenerateBoundingSphere(PrimitiveSurface& surface, BlitCL::DynamicArray<Vertex>& surfaceVertices, BlitCL::DynamicArray<uint32_t>& surfaceIndices)
    {
        BlitML::vec3 center{ 0.f };
        for (size_t i = 0; i < surfaceVertices.GetSize(); ++i)
        {
            center = center + surfaceVertices[i].position;
        }
        center = center / static_cast<float>(surfaceVertices.GetSize());

        // Bounding sphere radius
        float radius = 0;
        for (size_t i = 0; i < surfaceVertices.GetSize(); ++i)
        {
            const auto& pos = surfaceVertices[i].position;
            radius = BlitML::Max(radius, BlitML::Distance(center, BlitML::vec3(pos.x, pos.y, pos.z)));
        }
        surface.center = center;
        surface.radius = radius;
    }


    void GenerateTangents(BlitCL::DynamicArray<BlitzenEngine::Vertex>& vertices, BlitCL::DynamicArray<uint32_t>& indices)
    {
        for (size_t i = 0; i < indices.GetSize(); i += 3)
        {
            auto i0 = indices[i + 0];
            auto i1 = indices[i + 1];
            auto i2 = indices[i + 2];

            auto edge1 = vertices[i1].position - vertices[i0].position;
            auto edge2 = vertices[i2].position - vertices[i0].position;

            auto deltaU1 = float(vertices[i1].uvX - vertices[i0].uvX);
            auto deltaV1 = float(vertices[i1].uvY - vertices[i0].uvY);

            auto deltaU2 = float(vertices[i2].uvX - vertices[i0].uvX);
            auto deltaV2 = float(vertices[i2].uvY - vertices[i0].uvY);

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

            vertices[i0].tangentX = uint8_t(t4.x * 127.f + 127.5f);
            vertices[i0].tangentY = uint8_t(t4.y * 127.f + 127.5f);
            vertices[i0].tangentZ = uint8_t(t4.z * 127.f + 127.5f);
            vertices[i0].tangentW = uint8_t(t4.w * 127.f + 127.5f);

            vertices[i1].tangentX = uint8_t(t4.x * 127.f + 127.5f);
            vertices[i1].tangentY = uint8_t(t4.y * 127.f + 127.5f);
            vertices[i1].tangentZ = uint8_t(t4.z * 127.f + 127.5f);
            vertices[i1].tangentW = uint8_t(t4.w * 127.f + 127.5f);

            vertices[i2].tangentX = uint8_t(t4.x * 127.f + 127.5f);
            vertices[i2].tangentY = uint8_t(t4.y * 127.f + 127.5f);
            vertices[i2].tangentZ = uint8_t(t4.z * 127.f + 127.5f);
            vertices[i2].tangentW = uint8_t(t4.w * 127.f + 127.5f);
        }
    }

    void GenerateHlslVertices(MeshResources& context)
    {
        if (context.m_hlslVtxs.GetSize())
        {
            return;
        }

        for (size_t i = 0; i < context.m_vertices.GetSize(); ++i)
        {
            const auto& classic = context.m_vertices[i];
            HlslVtx hlsl{};

            hlsl.position = classic.position;

            hlsl.mappingU = classic.uvX;
            hlsl.mappingV = classic.uvY;

            hlsl.normals = classic.normalX << 24 | classic.normalY << 16 | classic.normalZ << 8 | classic.normalW;
            hlsl.tangents = classic.tangentX << 24 | classic.tangentY << 16 | classic.tangentZ << 8 | classic.tangentW;

            context.m_hlslVtxs.PushBack(hlsl);
        }
    }

    void LoadTestGeometry(MeshResources& context)
    {
        LoadMeshFromObj(context, "Assets/Meshes/dragon.obj", "dragon");
        LoadMeshFromObj(context, "Assets/Meshes/kitten.obj", "kitten");
        LoadMeshFromObj(context, "Assets/Meshes/FinalBaseMesh.obj", "human");
    }
}