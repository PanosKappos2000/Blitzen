#pragma once
#include "Renderer/Resources//renderingResourcesTypes.h"
#include "BlitCL/DynamicArray.h"
#include "BlitCL/blitHashMap.h"

namespace BlitzenEngine
{
    struct MeshResources
    {
        Mesh m_meshes[BlitzenCore::Ce_MaxMeshCount];
        BlitCL::HashMap<Mesh> m_meshMap;
        size_t m_meshCount = 0;

        // Mesh has one or more surfaces / Primitives
        BlitCL::DynamicArray<PrimitiveSurface> m_surfaces;
        BlitCL::DynamicArray<IsPrimitiveTransparent> m_bTransparencyList;

		// Surface has one ore more LODs (up to Ce_MaxLodCount)
        BlitCL::DynamicArray<LodData> m_LODs;
        BlitCL::DynamicArray<LodInstanceCounter> m_lodInstanceList;

        // Lod has one or more clusters (if they are generated)
        BlitCL::DynamicArray<Cluster> m_clusters;
        // Index buffer for cluster modes
        BlitCL::DynamicArray<uint32_t> m_clusterIndices;

        // Lod and cluster have multiple vertices
        BlitCL::DynamicArray<Vertex> m_vertices;
        BlitCL::DynamicArray<HlslVtx> m_hlslVtxs;
        // Index buffer for surface / draw modes
        BlitCL::DynamicArray<uint32_t> m_indices;

        BlitCL::DynamicArray<uint32_t> m_primitiveVertexCounts;

        bool AddMesh(uint32_t firstSurface, uint32_t surfaceCount, const char* meshName = "BLIT_DO_NOT_ADD_TO_MESH_TABLE");
    };

    bool LoadMeshFromObj(MeshResources& context, const char* filename, const char* meshName);

    // Loads a single primitive and adds it to the global array
    void GenerateSurface(MeshResources& context, BlitCL::DynamicArray<Vertex>& vertices, BlitCL::DynamicArray<uint32_t>& indices);

    // Generates LODs for the vertices of a given surface
    void GenerateLODs(MeshResources& context, PrimitiveSurface& surface, BlitCL::DynamicArray<Vertex>& surfaceVertices, BlitCL::DynamicArray<uint32_t>& surfaceIndices);

    // Generates clusters for a given array of vertices and indices
    size_t GenerateClusters(MeshResources& context, BlitCL::DynamicArray<Vertex>& vertices, BlitCL::DynamicArray<uint32_t>& indices, uint32_t vertexOffset);

    // Generates bounding sphere for primitive based on given vertices and indices
    void GenerateBoundingSphere(PrimitiveSurface& surface, BlitCL::DynamicArray<Vertex>& surfaceVertices, BlitCL::DynamicArray<uint32_t>& surfaceIndices);

    void GenerateTangents(BlitCL::DynamicArray<BlitzenEngine::Vertex>& vertices, BlitCL::DynamicArray<uint32_t>& indices);

    void GenerateHlslVertices(MeshResources& context);

    // Tester. Loads kitten, stanford dragon and a male human
    void LoadTestGeometry(MeshResources& context);
}